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
