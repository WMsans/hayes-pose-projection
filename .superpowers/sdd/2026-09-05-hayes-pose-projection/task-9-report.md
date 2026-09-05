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
