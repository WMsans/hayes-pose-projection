# Task 9 Report: Consolidated Coordinate Table and Error Analysis

## Status

Implemented Task 9 on the `feature/hayes-pose-projection` branch. The existing RAII offscreen-rendering scope and `--mode both` path are preserved. The `both`/`gt` paths now collect look-at and calibrated projections and emit coordinate and error tables; the legacy look-at-only rendering path remains unchanged.

## RED/GREEN evidence

### RED

Added `tests/test_tables.cpp` first, covering fixed coordinate formatting, the consolidated table, joint error output, and the required camera angular-error CSV. Before adding the production interface, this command failed as expected because the feature header did not exist:

```text
cmake -S . -B build && cmake --build build --target pose_tests -j
...
tests/test_tables.cpp:11:10: fatal error: 'tables.hpp' file not found
1 error generated.
make: *** [pose_tests] Error 2
```

### GREEN

After adding `src/tables.hpp`, `src/tables.cpp`, CMake registration, and the `main_project.cpp` integration:

```text
cmake --build build --target pose_tests -j && ./build/pose_tests
...
[doctest] test cases:  38 |  38 passed | 0 failed | 0 skipped
[doctest] assertions: 247 | 247 passed | 0 failed |
[doctest] Status: SUCCESS!
```

## Changed files

- `src/tables.hpp` — public coordinate/error table interfaces.
- `src/tables.cpp` — deterministic fixed-format CSV and TeX coordinate table writers, joint-error CSV, and mandated camera-angular-error CSV.
- `src/main_project.cpp` — collects both projection modes during the render pass and writes analysis artifacts while retaining RAII rendering.
- `tests/test_tables.cpp` — unit coverage for coordinate formatting and both error CSV outputs.
- `CMakeLists.txt` — registers the table implementation and tests.

## Verification

Full configure/build and the required both-mode run:

```text
cmake -S . -B build && cmake --build build -j
./build/pose-project --data data/Pose --out out --mode both
wrote 20 white renders, coordinate table and error analysis to "out"
```

Coordinate table and representative output:

```text
281 out/coords/all-2d-coordinates.csv
frame,camera,joint,name,u_lookat,v_lookat,u_gt,v_gt
0,55011271,0,Hip,502.33,517.54,561.73,564.13
0,55011271,1,RHip,480.25,503.83,539.75,550.09
```

Error output:

```text
frame  camera    mean_px  max_px  worst_joint  angular_deg
0      55011271  75.66    76.59   10           3.2559
1      58860488  187.82   189.88  13           8.4553
2      55011271  73.82    74.74   8            3.1462
3      58860488  134.05   135.39  3            5.8757
4      54138969  94.12    95.09   13           4.5648
```

The ruling-required angular CSV exists with deterministic four-decimal formatting:

```text
frame,camera,angular_deg
0,55011271,3.2559
1,58860488,8.4553
```

Artifact line counts:

```text
coordinate lines:      281
joint-error lines:       21
angular-error lines:     21
```

Determinism check:

```text
cp out/coords/all-2d-coordinates.csv /tmp/first.csv
./build/pose-project --data data/Pose --out out --mode both >/dev/null
 diff /tmp/first.csv out/coords/all-2d-coordinates.csv && echo "deterministic"
deterministic
```

Headline analysis check:

```text
mean of means: 97.2925 worst: 189.88
```

Full test suite:

```text
ctest --test-dir build --output-on-failure
100% tests passed out of 4
Total Test time (real) =  26.57 sec
```

`git diff --check` also completed without findings. The configure output contains only pre-existing dependency warnings from raylib/GLM/doctest; the project sources compile cleanly with the existing `-Wall -Wextra` flags.

## Self-review

- Coordinate rows are emitted in stable frame/joint order with `pose::fixed` formatting and the expected 20 × 14 rows plus header.
- Joint and angular analysis share the same frame/camera ordering and are written through the existing RAII-friendly CSV helper.
- `camera-angular-error.csv` uses the existing `angular_deg` and `camera_ids` data with no new calculation or interface.
- Output directories and file-open failures are handled consistently with existing I/O code.
- The existing `--mode lookat`, `--mode gt`, and `--mode both` CLI behavior remains valid; consolidated analysis is emitted when calibrated data is available (`gt` and `both`).
- No unrelated files or refactors were introduced.

## Concerns

No blocking concerns. CMake reports dependency deprecation/OpenGL warnings during configuration, inherited from vendored dependencies and unrelated to this change. Existing optional render tests also print expected raylib warnings for deliberately invalid/missing image inputs.

## Commit

The required commit is created with:

```text
git add -A
git commit -m "feat: consolidated 2D coordinate table and reprojection error analysis"
```

## Fix round 1/5 report

### Status

Fixed both open review findings. The both-mode CTest now validates all required table artifacts, exact line counts and headers, and every CSV row shape. Error-table writing now rejects every frame whose look-at or ground-truth projection does not contain exactly `kJointCount` (14) joints.

### Changed files

- `tests/check_pose_project_cli.cmake` — validates coordinate CSV (281 lines), TeX (287 lines), joint-error CSV (21 lines), camera-angular-error CSV (21 lines), headers, row shapes, and required artifact existence in the real both-mode data run.
- `src/tables.cpp` — validates both projection vectors per frame before calculating errors.
- `tests/test_tables.cpp` — covers malformed 1-, 13-, and 15-joint frames.

### TDD evidence

Focused regression before the production guard:

```bash
cmake --build build --target pose_tests -j && ./build/pose_tests --test-case='*rejects frames*'
```

```text
CHECK_THROWS_WITH ... did NOT throw at all! (3 failures)
```

After the guard:

```text
[doctest] test cases: 1 | 1 passed | 0 failed | 38 skipped
[doctest] assertions: 3 | 3 passed | 0 failed |
```

### Verification commands and output

```bash
set -e
cmake -S . -B build
cmake --build build -j
./build/pose_tests
ctest --test-dir build --output-on-failure
```

```text
[100%] Built target pose_tests
[doctest] test cases: 39 | 39 passed | 0 failed | 0 skipped
[doctest] assertions: 250 | 250 passed | 0 failed |
100% tests passed out of 4
Total Test time (real) = 26.55 sec
```

```bash
ctest --test-dir build -R pose_project_supports_both_mode --output-on-failure
```

```text
1/1 Test #4: pose_project_supports_both_mode ... Passed
100% tests passed out of 1
```

```bash
out=/tmp/hayes-pose-task-9-fix-round-1
rm -rf "$out" "$out.first"
./build/pose-project --data data/Pose --out "$out" --mode both
for path in \
  "$out/coords/all-2d-coordinates.csv" \
  "$out/coords/all-2d-coordinates.tex" \
  "$out/analysis/joint-error.csv" \
  "$out/analysis/camera-angular-error.csv"; do
  test -f "$path"
  wc -l < "$path"
done
head -2 "$out/coords/all-2d-coordinates.csv"
head -2 "$out/analysis/joint-error.csv"
head -2 "$out/analysis/camera-angular-error.csv"
head -3 "$out/coords/all-2d-coordinates.tex"
cp -R "$out" "$out.first"
./build/pose-project --data data/Pose --out "$out" --mode both >/dev/null
diff -rq "$out.first" "$out"
```

```text
wrote 20 white renders, coordinate table and error analysis to "/tmp/hayes-pose-task-9-fix-round-1"
281
287
21
21
frame,camera,joint,name,u_lookat,v_lookat,u_gt,v_gt
0,55011271,0,Hip,502.33,517.54,561.73,564.13
frame,camera,mean_px,max_px,worst_joint,angular_deg
0,55011271,75.66,76.59,10,3.2559
frame,camera,angular_deg
0,55011271,3.2559
\begin{longtable}{rlrlrrrr}
\toprule
Frame & Camera & \# & Joint & $u_{\text{look-at}}$ & $v_{\text{look-at}}$ & $u_{\text{gt}}$ & $v_{\text{gt}}$ \\
all both-mode artifacts identical
```

`git diff --check` completed without findings. Dependency deprecation/OpenGL warnings remain pre-existing configure/test output.

### Commit

```text
fix: harden Task 9 artifact and joint validation
```
