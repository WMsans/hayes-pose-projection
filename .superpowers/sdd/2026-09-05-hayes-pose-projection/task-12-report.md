# Task 12 Report: The Python reference check and the README

## Status

Implemented the requested Task 12 deliverables. No project source files or ignore rules were modified.

## Files

- `tools/reference_projection.py` — independent standard-library-only reference implementation. It loads `data/focal.txt` and `data/poses.txt`, reconstructs the camera basis and pinhole projection, then checks both methods in `out/coords_long.csv`.
- `tools/check_projection.sh` — executable end-to-end check for unit tests, build, Godot self-test, CSV export, and the Python reference.
- `README.md` — project usage, answers, projection-method explanation, checks, and repository layout.
- `.superpowers/sdd/2026-09-05-hayes-pose-projection/task-12-report.md` — this report, as requested.

The existing `.gitignore` remains unchanged; `.worktrees/` and `out/` remain ignored.

## TDD and verification commands

### TDD red phase

Temporary test (kept outside the repository at `/tmp/task12_artifact_test.py`) was run before implementation:

```text
$ python3 /tmp/task12_artifact_test.py .
Traceback (most recent call last):
  ...
AssertionError: missing required artifacts: tools/reference_projection.py, tools/check_projection.sh, README.md
```

### Artifact and static checks

```text
$ chmod +x tools/check_projection.sh
$ python3 /tmp/task12_artifact_test.py .
artifact contract passed
$ git diff --check
(clean; exit 0)
$ python3 -m py_compile tools/reference_projection.py
(clean; exit 0)
$ bash -n tools/check_projection.sh
(clean; exit 0)
```

The staged diff contained exactly the three requested implementation files, with the shell script mode `100755`.

### Full check

Command:

```text
./tools/check_projection.sh
```

Fresh captured result:

```text
== unit tests ==
Passed!  - Failed:     0, Passed:    33, Skipped:     0, Total:    33, Duration: 24 ms - PoseProjection.Tests.dll (net10.0)
== build ==
== in-engine self-test ==
self-test: OK, both projectors agree on all 280 points
== export tables ==
wrote tables to .../out
== python reference ==
OK: 560 rows match the reference, worst deviation 0.000670 px
all checks passed
```

The command exited with status `0`.

## Self-review

- The Python reference imports only `csv`, `math`, `os`, and `sys` from the standard library.
- It independently recomputes coordinates from the raw data rather than calling or importing the C# implementation.
- It validates the expected `2 * 20 * 14 = 560` long-form rows and reports per-row deviations against the `0.01` pixel tolerance.
- The shell script uses `set -e`, resolves and enters the repository root, runs every required check in the specified order, and is executable.
- The README includes the required run commands, challenge answers, both projection methods, viewport rationale, verification command, and layout.
- No `.worktrees/` or `out/` ignore rules were changed.

## Concerns

Godot emits 46 resource-load error lines for `res://data/frames/*.png` during the export invocation in this environment. Despite those messages, the process exits successfully, writes the CSV tables, the in-engine self-test passes, and the independent reference validates all 560 rows. This appears to be an existing headless resource-import/runtime issue outside the three requested Task 12 files; it is recorded rather than suppressed.

Interactive GUI control exercise and visual inspection of generated PNGs were not performed in this headless verification session.

## Fix round 1 report

### Root cause and changes

The resource errors were reproducible before the fix. `Main._Ready()` calls `ShowFrame(0)`, and `Exporter.WriteTables()` calls `ShowFrame()` once per frame. `ShowFrame()` unconditionally called `GD.Load<Texture2D>("res://data/frames/NN.png")`, even for `--self-test` and `--export-csv`. In this runtime `OS.HasFeature("headless")` was false while `DisplayServer.GetName()` was `headless`, so the confirmed headless guard uses the display-server name. GUI display servers still take the photograph-loading path.

Changed:

- `scripts/Main.cs`: skip photograph texture loading only when `DisplayServer.GetName() == "headless"`; projection refresh remains active.
- `tools/reference_projection.py`: require the exact CSV header and exact `(frame, joint_id, method)` coverage for frames `0..19`, joints `0..13`, and methods `manual_pinhole` and `godot_unproject`; reject duplicates, unknown methods, missing keys, and wrong row counts.
- `tools/check_projection.sh`: fail on Godot `ERROR:` output and validate the three exported table files and their expected line counts (`21`, `561`, `21`).

### Fresh verification

```text
$ dotnet build PoseProjection.csproj --nologo
Build succeeded.
0 Warning(s)
0 Error(s)

$ python3 /tmp/task12_round1_test.py .
round 1 regression probes passed

$ ./tools/check_projection.sh
== unit tests ==
Passed!  - Failed:     0, Passed:    33, Skipped:     0, Total:    33
== build ==
Build succeeded.
0 Warning(s)
0 Error(s)
== in-engine self-test ==
self-test: OK, both projectors agree on all 280 points
== export tables ==
wrote tables to .../out
== python reference ==
OK: 560 rows match the reference, worst deviation 0.000670 px
all checks passed
full-check-exit=0
```

The regression probe first failed before the fix on the headless PNG error. It also demonstrated the previous reference weakness: changing one row's method to the other valid method still returned exit `0`. After the fix, the same probe passed, including rejection of that mutated coverage. The artifact probe passed, `python3 -m py_compile tools/reference_projection.py`, `bash -n tools/check_projection.sh`, and `git diff --check` all exited `0`.

### GUI and image evidence

Attempted the required GUI run with `godot-mono --path .`. It started on the Metal display server and printed `pose projection: 20 frames, focal 1148.6`; after the local Godot editor import completed, the startup log contained no frame resource errors. The process could not be exercised interactively: this session has empty `DISPLAY`/`WAYLAND_DISPLAY`, no `xvfb-run` or `weston-run`, and `screencapture -x /tmp/task12-gui-initial.png` returned `could not create image from display`. The GUI process remained running until it was terminated after the timeout. Therefore no control exercise or visual confirmation of `proj_00.png`/`overlay_00.png` was possible.

Non-GUI artifact evidence: `out/coords_wide.csv` has 21 lines, `out/coords_long.csv` has 561 lines, and `out/method_agreement.csv` has 21 lines; all are non-empty and the Python reference validates all 560 coordinate rows. `find out -maxdepth 1 -type f -name '*.png'` returned no files, so the required PNG visual artifacts remain unverified in this environment. No generated import/UID files or other ignored editor state were included in the commit.
