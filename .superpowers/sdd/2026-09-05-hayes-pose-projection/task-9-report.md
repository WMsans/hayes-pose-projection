# Task 9 Report: The 2D projection overlay

## Status

Implemented Task 9 on top of Tasks 1–8.

## Files

- Created `scripts/ProjectionOverlay.cs`
  - Added the `ProjectionOverlay : Control` partial class.
  - Draws a white or photograph-backed 1000×1000 canvas.
  - Draws the projected bones and joints using `Framing`, `Bones`, and the requested fit/image-pixel behavior.
  - Photograph mode forces `ViewFraming.ImagePixels` to preserve registration.
- Modified `scripts/Main.cs`
  - Added overlay, view, projection method, frame index, default method, canvas, and projector fields.
  - Added `ShowFrame`, `ProjectCurrent`, `SetTwoDimensionalView`, and `LoadFrameTexture`.
  - Initializes the overlay after the fly camera and switches the initial frame setup to `ShowFrame(0)`.
  - Existing challenge-camera aiming, fly-camera setup, viewport settings, input policy, and camera marker/environment ordering were preserved.
- No other files were changed.

## Verification

### Build

Command:

```bash
dotnet build PoseProjection.csproj --nologo
```

Output:

```text
Build succeeded.
    0 Warning(s)
    0 Error(s)
```

### Headless self-test

The exact brief command could not run because this environment does not provide the `timeout` utility:

```text
/bin/bash: line 1: timeout: command not found
exit code: 127
```

The equivalent Godot Mono command was then run directly:

```bash
godot-mono --headless --path . -- --self-test
```

Output:

```text
self-test: OK, both projectors agree on all 280 points
exit code: 0
```

### Temporary visual smoke check

Temporarily added `SetTwoDimensionalView(true);` at the end of `_Ready`, then ran the project with:

```bash
godot-mono --path .
```

Godot Mono started successfully with the Metal renderer and emitted no runtime errors during the smoke interval. The process was stopped intentionally after 8 seconds. The temporary line was removed afterward.

Final checks confirmed:

```text
git diff --check: clean
temporary smoke line: absent
.worktrees/: ignored by .gitignore
```

A final build and headless self-test were rerun after removing the temporary line and both passed as recorded above.

## Self-review

- [x] `ProjectionOverlay` public API matches the brief.
- [x] Drawing uses `Main.ImageSize`, `Framing.Compute`, `Bones.All`, and `Bones.ColorFor` as specified.
- [x] Photograph drawing and image-pixel registration behavior match the brief.
- [x] Frame indices are clamped and frame changes update the figure, camera, photograph, and overlay points.
- [x] Manual and Godot projector selection matches `ProjectionMethod`.
- [x] 2D mode preserves camera switching and makes the mouse visible.
- [x] Overlay is full-rect, ignores mouse input, and starts hidden.
- [x] Existing camera marker/environment setup and viewport lock were not changed.
- [x] `.worktrees/` remains ignored.
- [x] No unrelated files or behavior were changed.

## Concerns

No code concerns found. The only environment limitation was the missing `timeout` shell utility; direct Godot Mono self-test execution provided the required successful result. The graphical smoke check was process/log based because no pixel-capture tool was available in the environment.

## Commit

`Draw the projected skeleton in a 2D overlay with both framings` (the report is included in this commit).

## Fix Round 1: Review findings

### Findings fixed

- Moved `ShowFrame(0)` until after `Method`, `_godot`, `Overlay`, and the canvas attachment are initialized. The initial frame now loads `00.png` into `Overlay.Photograph` and refreshes its projected points before the fly camera is snapped.
- Added `Main.SetProjectionMethod(ProjectionMethod method)`. It updates the public `Method` field and refreshes `Overlay.Points` whenever 2D mode is active. This keeps the existing public field readable for later controls while providing a refresh-safe setter path.
- Preserved the later startup ordering: `ShowFrame(0)` aims the challenge camera before `Fly.SnapTo(ChallengeCam)`, and the command-line self-test remains after setup.

### TDD focused regression checks

Initial-frame ordering red phase, before the fix:

```text
initial ShowFrame index=598, overlay add index=1101
AssertionError: initial ShowFrame must run after overlay creation
```

After moving the call:

```text
initial ShowFrame index=1064, overlay add index=1034
initial overlay ordering: OK
```

Projection-method refresh-path red phase, before the setter:

```text
AssertionError: missing projection-method setter
```

After adding the setter:

```text
projection method refresh path: OK
```

### Verification

Covering build:

```bash
dotnet build PoseProjection.csproj --nologo
```

Output:

```text
Build succeeded.
    0 Warning(s)
    0 Error(s)
```

Existing unit tests:

```bash
dotnet test tests/PoseProjection.Tests/PoseProjection.Tests.csproj --nologo
```

Output:

```text
Passed!  - Failed:     0, Passed:    33, Skipped:     0, Total:    33
```

Headless asset import prerequisite:

```bash
godot-mono --headless --path . --editor --quit
```

Completed with exit code 0. The generated local import metadata is ignored/removed after verification.

Headless self-test:

```bash
godot-mono --headless --path . -- --self-test
```

Output:

```text
self-test: OK, both projectors agree on all 280 points
headless self-test exit code: 0
```

Focused smoke/static check:

```text
focused Task 9 fix smoke/static: OK
git diff --check: clean
```

The check confirms the startup order, photograph assignment before overlay refresh, public `Method` API, and active-mode setter refresh guard.

Interactive launch smoke:

```text
Godot Engine v4.7.2.stable.mono.official.ed1daf0bf
Metal 4.0 - Forward+ - Using Device #0: Apple - Apple M1 (Apple7)
pose projection: 20 frames, focal 1148.6
interactive smoke exit after controlled stop: -15
```

### Self-review

- [x] Initial frame setup occurs after the overlay and both projectors exist.
- [x] `Overlay.Photograph` is assigned from frame `00.png` during startup.
- [x] The setter preserves `public ProjectionMethod Method` and refreshes only an active 2D overlay.
- [x] Fly-camera snap still follows the initial frame aim.
- [x] No unrelated tracked files changed.
- [x] `git diff --check` is clean.

### Concerns

The environment does not provide the `timeout` utility, so direct Godot commands and a controlled Python launch were used. Godot required a one-time editor import before runtime PNG loading; generated `.import`/`.uid` files were removed and are not part of the change.
