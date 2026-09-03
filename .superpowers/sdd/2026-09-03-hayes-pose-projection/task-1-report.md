# Task 1 Report

## Status

Implemented the Task 1 project scaffold, pinned FetchContent dependencies, smoke test, calibration vendor files, data-fetch script, and README. Preserved the existing `.worktrees/` ignore rule per controller clarification. Challenge data remains ignored and is not included in the task commit.

## Preflight and data

- `pkg-config --exists x11 && echo "x11 ok" || echo "MISSING x11"` -> `x11 ok`
- `ls /usr/include/GL/gl.h && echo "gl ok" || echo "MISSING mesa headers"` -> `MISSING mesa headers` (`ls: ... No such file or directory`)
- No system packages were installed. The missing Linux Mesa header does not block this macOS build; raylib configured and compiled using the available platform backend.
- Calibration fetch succeeded. Validation printed `['54138969', '55011271', '58860488', '60457274']`.
- `./scripts/fetch-data.sh` succeeded with `data ready: data/Pose`; `data/` is ignored and was not staged.

## Verification

- `cmake -S . -B build && cmake --build build -j` -> exit 0; targets `raylib`, `pose_core`, and `pose_tests` built. Dependency compilation emitted existing third-party warnings.
- `./build/pose_tests` -> 1 test case and 1 assertion passed; 0 failed.
- `scripts/fetch-data.sh` is executable.

## Files

- `CMakeLists.txt`
- `cmake/Dependencies.cmake`
- `.gitignore`
- `README.md`
- `scripts/fetch-data.sh`
- `src/placeholder.cpp`
- `tests/test_smoke.cpp`
- `third_party/h36m/camera-parameters.json`
- `third_party/h36m/LICENSE`
