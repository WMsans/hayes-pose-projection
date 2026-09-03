# Task 6 Report: Analysis metrics and CSV output

## Status

Implemented Task 6 using TDD. Added deterministic joint reprojection error, angular rotation error, limb-length, fixed-decimal formatting, and CSV output APIs. Added the analysis unit tests and registered the new source and test files in CMake.

## RED evidence

Added the required tests to `tests/test_analysis.cpp` and registered `src/analysis.cpp` and the test file in `CMakeLists.txt`, then ran:

```text
$ cmake -S . -B build && cmake --build build -j 2>&1 | tail -3
...
CMake Error at CMakeLists.txt:12 (target_sources):
  Cannot find source file:

    src/analysis.cpp

CMake Error at CMakeLists.txt:11 (add_library):
  No SOURCES given to target: pose_core

CMake Generate step failed.  Build files cannot be regenerated correctly.
```

The build failed because the production source required by the newly registered target did not yet exist.

## GREEN evidence

After implementing the analysis API and source, the focused analysis test cases passed:

```text
$ ./build/pose_tests --test-case='joint_error*'
[doctest] test cases: 1 | 1 passed | 0 failed | 27 skipped

$ ./build/pose_tests --test-case='angular_error*'
[doctest] test cases: 2 | 2 passed | 0 failed | 26 skipped

$ ./build/pose_tests --test-case='limb_lengths*'
[doctest] test cases: 1 | 1 passed | 0 failed | 27 skipped

$ ./build/pose_tests --test-case='fixed*'
[doctest] test cases: 1 | 1 passed | 0 failed | 27 skipped

$ ./build/pose_tests --test-case='write_csv*'
[doctest] test cases: 1 | 1 passed | 0 failed | 27 skipped
```

The build and full unit suite passed:

```text
$ cmake --build build -j && ./build/pose_tests
[100%] Built target pose_tests
[doctest] test cases:  28 |  28 passed | 0 failed | 0 skipped
[doctest] assertions: 210 | 210 passed | 0 failed |
[doctest] Status: SUCCESS!

$ ctest --test-dir build --output-on-failure
1/1 Test #1: unit .............................   Passed    0.02 sec
100% tests passed out of 1
```

`git diff --check` also completed successfully with no output.

## Changed files

- `src/analysis.hpp`: added `FrameError`, analysis metric declarations, deterministic formatting, and CSV writer declarations.
- `src/analysis.cpp`: implemented joint pixel error, clamped rotation-angle error in degrees, skeleton bone lengths, classic-locale fixed formatting, and newline-delimited CSV output with row validation.
- `tests/test_analysis.cpp`: added the six required metric and CSV behavior tests.
- `CMakeLists.txt`: added the analysis source to `pose_core` and the analysis tests to `pose_tests`.

## Self-review

- `joint_error` rejects empty or size-mismatched inputs, computes Euclidean pixel distances, reports the mean and maximum, and retains the first worst-joint index on ties.
- `angular_error_degrees` computes the relative rotation trace, clamps floating-point drift before `acos`, and uses a portable C++20-compatible pi constant instead of `M_PI`.
- `limb_lengths` uses the existing `bones()` definition, returns all 13 lengths, and depends only on joint differences, so translation does not affect results.
- `fixed` uses `std::locale::classic()`, fixed notation, and the requested precision for deterministic CSV values.
- `write_csv` creates parent directories, writes the header and rows with LF endings, rejects mismatched row widths, and leaves values unquoted as specified.
- Existing pose I/O, skeleton, and camera APIs were not changed.
- The project compiled under strict C++20 with the existing warning flags.

## Concerns

No known functional concerns for the specified interfaces and CSV schema. CSV fields are intentionally emitted without escaping or quoting, matching the required quoted-free output contract.

## Fix evidence — round 1

### Finding addressed

`write_csv` attempted `create_directories` with an empty `parent_path()` for bare output filenames such as `results.csv`, causing a filesystem exception before opening the file.

### Fix

- `src/analysis.cpp`: guard `create_directories` so it runs only when `path.parent_path()` is non-empty.
- `tests/test_analysis.cpp`: add `write_csv accepts a bare output path`, which writes and reads `test_analysis_bare.csv` and removes it afterward.

### Covering test file and test case

`tests/test_analysis.cpp` — `TEST_CASE("write_csv accepts a bare output path")`.

### Exact verification commands and output

RED reproduction before the production fix:

```text
$ cmake --build build -j 2>&1 && ./build/pose_tests --test-case='write_csv accepts a bare output path'
[  4%] Built target glm
[ 54%] Built target glfw
[ 73%] Built target raylib
[ 85%] Built target pose_core
[ 88%] Building CXX object CMakeFiles/pose_tests.dir/tests/test_analysis.cpp.o
[ 90%] Linking CXX executable pose_tests
[100%] Built target pose_tests
[doctest] doctest version is "2.4.11"
[doctest] run with "--help" for options
===============================================================================
/Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection/tests/test_analysis.cpp:53:
TEST CASE:  write_csv accepts a bare output path

/Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection/tests/test_analysis.cpp:53: ERROR: test case THREW exception: filesystem error: in create_directories: No such file or directory [""]

===============================================================================
[doctest] test cases: 1 | 0 passed | 1 failed | 28 skipped
[doctest] assertions: 0 | 0 passed | 0 failed |
[doctest] Status: FAILURE!

Command exited with code 1
```

GREEN analysis-focused and full test run:

```text
$ cmake --build build -j && ./build/pose_tests --test-case='joint_error*' && ./build/pose_tests --test-case='angular_error*' && ./build/pose_tests --test-case='limb_lengths*' && ./build/pose_tests --test-case='fixed*' && ./build/pose_tests --test-case='write_csv*' && ./build/pose_tests
[  4%] Built target glm
[ 54%] Built target glfw
[ 73%] Built target raylib
[ 76%] Building CXX object CMakeFiles/pose_core.dir/src/analysis.cpp.o
[ 78%] Linking CXX static library libpose_core.a
[ 85%] Built target pose_core
[ 88%] Linking CXX executable pose_tests
[100%] Built target pose_tests
[doctest] doctest version is "2.4.11"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases: 1 | 1 passed | 0 failed | 28 skipped
[doctest] assertions: 3 | 3 passed | 0 failed |
[doctest] Status: SUCCESS!
[doctest] doctest version is "2.4.11"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases: 2 | 2 passed | 0 failed | 27 skipped
[doctest] assertions: 2 | 2 passed | 0 failed |
[doctest] Status: SUCCESS!
[doctest] doctest version is "2.4.11"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases:  1 | 1 passed | 0 failed | 28 skipped
[doctest] assertions: 26 | 26 passed | 0 failed |
[doctest] Status: SUCCESS!
[doctest] doctest version is "2.4.11"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases: 1 | 1 passed | 0 failed | 28 skipped
[doctest] assertions: 2 | 2 passed | 0 failed |
[doctest] Status: SUCCESS!
[doctest] doctest version is "2.4.11"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases: 2 | 2 passed | 0 failed | 27 skipped
[doctest] assertions: 2 | 2 passed | 0 failed |
[doctest] Status: SUCCESS!
[doctest] doctest version is "2.4.11"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases:  29 |  29 passed | 0 failed | 0 skipped
[doctest] assertions: 211 | 211 passed | 0 failed |
[doctest] Status: SUCCESS!
```

The exact command exited 0. `git diff --check` completed with no output, and `test_analysis_bare.csv` was cleaned up.
