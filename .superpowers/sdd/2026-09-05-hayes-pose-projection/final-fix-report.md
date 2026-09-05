# Final Review Fix Wave Report

## Status

Implemented the approved final-review fixes:

- Hardened `SelfTest` so both manual and Godot projector outputs must be finite, agree within tolerance, and remain inside the image bounds.
- Made missing photographs, failed viewport captures, and non-OK PNG saves throw export errors; the existing image-export state restoration remains in `finally`.
- Added and updated the 3D camera-marker-to-centroid line for every displayed frame.
- Corrected the README agreement claim to the measured maximum difference of `0.000183 px`.

## Files changed

- `scripts/SelfTest.cs`
- `scripts/Exporter.cs`
- `scripts/ControlPanel.cs`
- `scripts/Main.cs`
- `README.md`

## TDD focused probe

The temporary source-contract probe was written before implementation and failed against the baseline:

```text
$ python3 /tmp/final_fix_wave_test.py .
FAIL: SelfTest checks finite output; SelfTest checks Godot range; Exporter checks PNG result; Exporter checks photograph availability; UI surfaces export failures; Main has camera aim line; README reports measured agreement
```

After implementation:

```text
$ python3 /tmp/final_fix_wave_test.py .
final fix-wave focused probes passed
focused-probe-exit=0
```

## Focused verification

```text
$ git diff --check
 diff-check-exit=0
```

```text
$ dotnet build PoseProjection.csproj --nologo
  Determining projects to restore...
  All projects are up-to-date for restore.
  PoseProjection -> /Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection-implementation/.godot/mono/temp/bin/Debug/PoseProjection.dll

Build succeeded.
    0 Warning(s)
    0 Error(s)

Time Elapsed 00:00:00.50
```

```text
$ (cd tests/PoseProjection.Tests && dotnet test --nologo)
  Determining projects to restore...
  All projects are up-to-date for restore.
  PoseProjection.Tests -> /Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection-implementation/tests/PoseProjection.Tests/bin/Debug/net10.0/PoseProjection.Tests.dll
Test run for /Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection-implementation/tests/PoseProjection.Tests/bin/Debug/net10.0/PoseProjection.Tests.dll (.NETCoreApp,Version=v10.0)
A total of 1 test files matched the specified pattern.

Passed!  - Failed:     0, Passed:    33, Skipped:     0, Total:    33, Duration: 23 ms - PoseProjection.Tests.dll (net10.0)
```

The prescribed `timeout` wrapper was unavailable (`/bin/bash: timeout: command not found`), so the self-test was rerun directly:

```text
$ godot-mono --headless --path . -- --self-test
Godot Engine v4.7.2.stable.mono.official.ed1daf0bf - https://godotengine.org

self-test: OK, both projectors agree on all 280 points
self-test exit code: 0
```

## Full verification

```text
$ ./tools/check_projection.sh
== unit tests ==
  Determining projects to restore...
  All projects are up-to-date for restore.
  PoseProjection.Tests -> /Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection-implementation/tests/PoseProjection.Tests/bin/Debug/net10.0/PoseProjection.Tests.dll
Test run for /Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection-implementation/tests/PoseProjection.Tests/bin/Debug/net10.0/PoseProjection.Tests.dll (.NETCoreApp,Version=v10.0)
A total of 1 test files matched the specified pattern.

Passed!  - Failed:     0, Passed:    33, Skipped:     0, Total:    33, Duration: 22 ms - PoseProjection.Tests.dll (net10.0)
== build ==
  Determining projects to restore...
  All projects are up-to-date for restore.
  PoseProjection -> /Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection-implementation/.godot/mono/temp/bin/Debug/PoseProjection.dll

Build succeeded.
    0 Warning(s)
    0 Error(s)

Time Elapsed 00:00:00.50
== in-engine self-test ==
Godot Engine v4.7.2.stable.mono.official.ed1daf0bf - https://godotengine.org

self-test: OK, both projectors agree on all 280 points
== export tables ==
Godot Engine v4.7.2.stable.mono.official.ed1daf0bf - https://godotengine.org

wrote tables to /Users/jeremyzhao/Development/python/uci-faculty-crawer/challenges/hayes-pose-projection/.worktrees/hayes-pose-projection-implementation/out
== python reference ==
OK: 560 rows match the reference, worst deviation 0.000670 px
all checks passed
```

`./tools/check_projection.sh` exited `0`.

## Self-review

Reviewed the committed fix scope for the four final-review findings. The export failure paths throw actionable errors, `ControlPanel.OnExport` displays and logs those errors, and `WriteImages` retains all state restoration in its `finally` block. Both projector points are checked before any difference arithmetic, and both are range-checked. The camera line is rebuilt from the current camera position to the current frame centroid whenever the frame changes. No unrelated files were changed.

## Concerns

Interactive GUI export and visual PNG inspection remain unavailable in this headless session because `DISPLAY`/`WAYLAND_DISPLAY` are unset. The headless CSV/self-test path and all engine-free tests pass.
