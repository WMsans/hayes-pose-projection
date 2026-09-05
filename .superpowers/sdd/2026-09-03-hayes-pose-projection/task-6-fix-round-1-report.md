# Task 6 Fix Report — round 1

## Finding addressed

`write_csv` attempted `create_directories` with an empty `parent_path()` for bare output filenames such as `results.csv`, causing a filesystem exception before opening the file.

## Fix

- `src/analysis.cpp`: guard `create_directories` so it runs only when `path.parent_path()` is non-empty.
- `tests/test_analysis.cpp`: add `write_csv accepts a bare output path`, which writes and reads `test_analysis_bare.csv` and removes it afterward.

## Covering test

`tests/test_analysis.cpp` — `TEST_CASE("write_csv accepts a bare output path")`.

## Verification

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
[doctest] test cases:  1 |  1 passed | 0 failed | 28 skipped
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
