# Task 10 Report: The control panel

## Status

Implemented Task 10 on top of Tasks 1–9.

## Files

- Created `scripts/ControlPanel.cs`
  - Added the runtime-built control panel for frame stepping, view selection, projection method selection, framing, background, camera snapping, export status, and readouts.
  - Kept keyboard shortcuts out of the panel so the keyboard remains owned by `FlyCamera`.
  - Method selection calls `Main.SetProjectionMethod(...)`, preserving live 2D overlay refresh.
- Modified `scripts/Main.cs`
  - Added the public `ControlPanel Panel` field.
  - Added the panel to the canvas and called `Panel.Build(this)` after `ShowFrame(0)` and the fly-camera snap/current setup.
  - Added the temporary asynchronous `RunExport` stub.
- Created `task-10-report.md` (this report).
- No other source files were changed. `.worktrees/` remains ignored.

## Verification

### TDD contract check — RED

A focused contract test was written before the production implementation at `/tmp/task10_contract_test.py` and run with:

```bash
python3 /tmp/task10_contract_test.py
```

It failed as expected because the new file did not yet exist:

```text
AssertionError: ControlPanel.cs is missing
```

### TDD contract check — GREEN

After implementing the panel and Main wiring, the same test was rerun:

```bash
python3 /tmp/task10_contract_test.py
```

Output:

```text
Task 10 contract: OK
```

This check covers the panel API, the `SetProjectionMethod` method-selection path (and rejects direct `Method` assignment), the export stub, panel field, and the required startup ordering.

### Build

```bash
dotnet build PoseProjection.csproj --nologo
```

Output:

```text
Build succeeded.
    0 Warning(s)
    0 Error(s)
```

### Existing unit tests

```bash
dotnet test tests/PoseProjection.Tests/PoseProjection.Tests.csproj --nologo
```

Output:

```text
Passed!  - Failed:     0, Passed:    33, Skipped:     0, Total:    33
```

### Headless Godot Mono self-test

The first direct run exposed only an uninitialized local asset-import cache (`No loader found for resource: res://data/frames/00.png`); the self-test still reported its expected result. Godot Mono’s headless editor import was then run:

```bash
godot-mono --headless --path . --editor --quit
```

It completed with exit code 0. The clean rerun was:

```bash
godot-mono --headless --path . -- --self-test
```

Output:

```text
self-test: OK, both projectors agree on all 280 points
```

Generated `.import` and `.uid` files were removed afterward because they are local generated artifacts and are not part of this task.

### Interactive launch smoke

Because this environment has no `timeout` utility, the project was launched with Godot Mono directly, allowed to run for 10 seconds, and stopped intentionally:

```bash
godot-mono --path .
```

Log output included:

```text
Metal 4.0 - Forward+ - Using Device #0: Apple - Apple M1 (Apple7)
pose projection: 20 frames, focal 1148.6
```

### Final hygiene

```text
git diff --check: clean
git status: only scripts/Main.cs and scripts/ControlPanel.cs changed before report/commit staging
.worktrees/: still ignored by .gitignore
```

## Self-review

- [x] `ControlPanel : PanelContainer` exposes `Build(Main)`, `UpdateReadouts()`, and `SetStatus(string)`.
- [x] Frame controls call `ShowFrame` and refresh readouts.
- [x] View, framing, background, snap, and export controls call the required Main/overlay/camera APIs.
- [x] Projection method selection calls `Main.SetProjectionMethod(...)`, not direct `Method` assignment.
- [x] Agreement readout uses both projectors and `CoordinateTable.MaxAbsDiff`.
- [x] Photograph mode disables the framing control.
- [x] `_Ready` order is `Overlay`, `Panel`, `ShowFrame(0)`, fly-camera snap/current, `Panel.Build(this)`, command-line loop, print.
- [x] The temporary `RunExport` stub waits one process frame and returns the specified message.
- [x] Prior camera, overlay, keyboard, and environment behavior was preserved.
- [x] `.worktrees/` ignore safety was preserved.

## Concerns

No code concerns found. The environment lacks the requested `timeout` shell utility, so direct Godot Mono execution and controlled process termination were used for the interactive smoke check. Full click-through UI behavior was not automated; the headless startup and runtime log completed without errors after asset import.
