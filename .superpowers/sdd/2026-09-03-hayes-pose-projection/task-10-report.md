# Task 10 Report: pose-explorer

## Status

Implemented and committed the optional plain raylib/raygui debug explorer.

Commit: `fb569be` (`feat: plain raylib debug explorer with 2D and 3D views`)

## Implementation

- Added `src/main_explorer.cpp`.
  - Loads poses, focal length, and S1 calibration from the existing shared APIs.
  - Provides a 1000x1000 single-view window with a deliberately plain raygui panel.
  - Supports frame selection, photo opacity, 2D/3D view switching, GT/look-at projection switching, photo/white background switching, and figure export.
  - Uses `pose::project_frame`, `pose::draw_skeleton_2d`, `pose::joint_error`, `pose::bones`, and the existing camera helpers.
  - Shows published and look-at camera frusta in 3D using red and blue colors.
  - Keeps photo unloading on frame changes and at shutdown.
- Added the `pose-explorer` executable in `CMakeLists.txt`, linked only against `pose_core`.
- Did not add a distortion toggle: the explicit abbreviated Task 10 source omits it, and adding a public API or broader behavior was unnecessary. Existing projection behavior remains unchanged.

## TDD / verification evidence

Initial red integration check, before implementation:

```text
cmake --build build --target pose-explorer
make: *** No rule to make target `pose-explorer'.  Stop.
```

Fresh verification after implementation:

- `cmake -S . -B build && cmake --build build -j`: succeeded and linked `pose-explorer`.
- `./build/pose_tests`: 39/39 test cases passed; 250/250 assertions passed.
- `ctest --test-dir build --output-on-failure`: 4/4 tests passed.
- `./build/pose-project --data data/Pose --out out --mode both`: succeeded and wrote 20 renders, coordinate tables, and error analysis.
- `xvfb-run -a ./build/pose-project --data data/Pose --out out --mode both`: could not run because `xvfb-run` is not installed in this environment (exit 127). The unwrapped batch command succeeded on macOS.
- `git diff --cached --check`: passed.

## Concerns

- Interactive display verification of slider/toggle behavior and `explorer-00.png` export was not possible because no `xvfb-run` command is available here and the session has no suitable automated GUI harness.
- The build emits warnings from the vendored `raygui.h` implementation under the existing `-Wall -Wextra` flags; there are no project-source compile errors.
- Generated `out/` artifacts remain ignored by the repository rules.

## Fix report — round 1/5

### Findings fixed

- Added optional `apply_distortion = false` to `pose::project_frame`; batch callers and table outputs retain the previous undistorted default, while the explorer explicitly controls GT distortion.
- Replaced the explorer's two-state projection flag with a cycling `lookat` / `gt` / `both` mode; `both` draws both projections in the single full-window 2D view.
- Added window and photo RAII scopes so exceptions and failed window initialization close the window and unload textures safely.
- Detect failed photo `LoadTexture` calls with `IsTextureValid`; failed frames are not marked loaded and are retried.

### Files changed

- `src/main_explorer.cpp`
- `src/render.hpp`
- `src/render.cpp`
- `tests/test_render.cpp`
- `.superpowers/sdd/2026-09-03-hayes-pose-projection/task-10-report.md`

### Covering tests

- `tests/test_render.cpp`: added `project_frame optionally applies identified camera distortion`, covering explicit GT distortion on/off and preserving the undistorted result.
- Existing `tests/test_render.cpp` projection, render, and error-path cases remained green; full `pose_tests` and CTest coverage were rerun.

### Exact verification commands and output

```text
cmake -S . -B build && cmake --build build -j
[100%] Built target pose-project
[100%] Built target pose-explorer
[100%] Built target pose_tests

./build/pose_tests
[doctest] test cases:  40 |  40 passed | 0 failed | 0 skipped
[doctest] assertions: 253 | 253 passed | 0 failed |
[doctest] Status: SUCCESS!

ctest --test-dir build --output-on-failure
100% tests passed out of 4
Total Test time (real) =  26.55 sec

./build/pose-project --data data/Pose --out out --mode both
wrote 20 white renders, coordinate table and error analysis to "out"
white=20, overlay=20, panel=20, coords=present, analysis=present

git diff --check
(output empty; exit 0)
```

### Concerns

- Interactive GUI input and screenshot export were not automated; `xvfb-run` is unavailable in this environment.
- The explorer build still emits existing warnings from vendored `raygui.h`; no project-source compile errors occurred.
