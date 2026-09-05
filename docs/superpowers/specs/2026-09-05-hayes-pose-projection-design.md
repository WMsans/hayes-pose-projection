# Hayes Pose Projection — Design

Date: 2026-09-05

## 1. Context

Wayne Hayes' research-student page (<https://ics.uci.edu/~wayne/research/students/>) sets a
challenge under the project *Human Pose Estimation via AI/ML*. The challenge itself has no
AI/ML component; it is purely geometric.

`Pose.zip` from that page contains:

- `frames/00.png` … `frames/19.png` — 20 photographs, 1000×1000 RGB.
- `focal.txt` — a single number, `1148.6`.
- `joint-names.txt` — 14 joints, IDs 0–13.
- `poses.txt` — 20 lines of 45 tab-separated columns: 3 camera-position values followed by
  14 joint triplets.
- `pose-sample.png` — the reference figure: a 2D skeleton overlaid on a photo (left) beside a
  3D skeleton plot (right).

The tasks set are:

- **(a)** find a camera orientation pointing from the given camera position toward the subject;
- **(b)** project the 3D skeleton to 2D, resembling the sample;
- **(c)** accumulate every 2D coordinate into one table for verification;
- **(d)** *(degree-holders only, explicitly optional)* superimpose the skeleton onto the frames.

We do all four.

## 2. Goals and non-goals

**Goals**

- Answer (a)–(d) with artefacts a reviewer can check without running anything.
- Ship it as an interactive Godot tool: fly around the 3D scene with WASD and the mouse, step
  through the 20 frames, and toggle to the 2D projection.
- Implement the projection **twice** — by hand and via Godot's camera — and let the user switch
  between them at runtime, so each independently verifies the other.

**Non-goals**

- No AI/ML, no pose estimation. The 3D poses are given.
- No attempt to recover the true camera extrinsics of the original capture. The challenge text
  explicitly says the reconstructed viewpoint need not match the photograph exactly.
- No export presets or packaged builds. The tool is run from source.

## 3. Environment (verified)

| Component | Version | Note |
| --- | --- | --- |
| Godot | `4.7.2.stable.mono.arch_linux` | invoked as `godot-mono`; the plain `godot` binary has no C# support |
| .NET SDK | 10.0.111 | only the 10.0.11 runtime is present |
| `Godot.NET.Sdk` | 4.7.2 | restores from nuget.org |
| Target framework | `net8.0` | Godot's `GodotPlugins.runtimeconfig.json` sets `rollForward: LatestMajor`, so net8.0 assemblies run on the .NET 10 runtime |
| CsvHelper | 33.1.0 | verified to build and run under `godot-mono --headless` |

`EnableDynamicLoading` must stay `true` in the csproj or NuGet dependencies are not copied
beside the assembly.

## 4. Data interpretation

### 4.1 Units and axes

Joint and camera coordinates are millimetres in a **Z-up, right-handed** world. Evidence: in
frame 00 the neck sits at z ≈ 631 and the ankles at z ≈ 8–48, so z = 0 is the floor; the camera
is at z ≈ 1606, a normal tripod height.

### 4.2 Focal length

`focal.txt` is labelled millimetres, but 1148.6 is the focal length in **pixels** — this is the
Human3.6M dataset, whose cameras have f ≈ 1145 px. Treating it as millimetres would require a
sensor size that is not supplied. We therefore use a pinhole model with f = 1148.6 px and the
principal point at the image centre, (500, 500), for the 1000×1000 frames. The resulting field
of view is 2·atan(500/1148.6) = **47.048°**, which is consistent with the frames.

### 4.3 Joints and bones

```
0 Hip   1 RHip  2 RKnee  3 RAnkle  4 LHip  5 LKnee  6 LAnkle
7 Neck  8 LUpperArm  9 LElbow 10 LWrist 11 RUpperArm 12 RElbow 13 RWrist
```

There is no head joint. Thirteen bones connect them:

| Chain | Edges | Colour |
| --- | --- | --- |
| Right leg | 0–1, 1–2, 2–3 | red |
| Left leg | 0–4, 4–5, 5–6 | blue |
| Spine | 0–7 | black |
| Left arm | 7–8, 8–9, 9–10 | blue |
| Right arm | 7–11, 11–12, 12–13 | red |

This matches the red/blue/black scheme in `pose-sample.png`.

### 4.4 Coordinate conversion

Godot is Y-up, right-handed, −Z forward. World → Godot is

```
godot = new Vector3(x, z, -y) / 1000f
```

which preserves handedness (determinant +1) and converts millimetres to metres.

`PoseFrame` stores **both** representations:

- `JointsWorld[14]` — raw Z-up millimetres, exactly as parsed;
- `JointsGodot[14]` — the converted metres.

The redundancy is deliberate. The manual projector consumes only `JointsWorld` and so never
touches the scene graph or Godot's camera, making it a genuinely independent implementation
rather than a wrapper around the same code path.

## 5. Camera orientation (task a)

For each frame:

```
target  = mean of the 14 joint positions
forward = normalize(target - cameraPosition)
```

with the camera's up vector held in the world-vertical plane, i.e. zero roll.

The centroid is used rather than the Hip because several frames are crouched or reaching, and
the centroid keeps the whole figure framed. The degenerate case for a look-at with a world-up
reference — looking straight down — cannot arise here: the camera is ~1.6 m up and ~5–6 m away,
so `forward` is never near-vertical.

In Godot this is `ChallengeCam.LookAt(targetGodot, Vector3.Up)`. In the manual projector it is
built explicitly (§7.1).

## 6. Viewport lock

The root viewport is pinned to exactly 1000×1000:

```
display/window/size/viewport_width  = 1000
display/window/size/viewport_height = 1000
display/window/size/resizable       = false
display/window/stretch/mode         = "canvas_items"
display/window/stretch/aspect       = "keep"
```

With `stretch/mode = canvas_items` the root viewport keeps its configured size no matter what
the OS window does, so `unproject_position` always returns literal image pixels. Without this
lock, method 2's numbers would silently depend on window size — which would make the exported
table unverifiable.

`ChallengeCam` uses `KeepAspect = KeepAspectEnum.Height` and `Fov = 47.048°`. On a square
viewport horizontal and vertical FOV coincide, but the setting is made explicit so the value is
unambiguous.

## 7. The two projection methods

```csharp
public enum ProjectionMethod { ManualPinhole, GodotUnproject }

public interface IProjector
{
    void Begin(PoseFrame frame, float focal);
    Vector2 Project(int jointIndex);
}
```

### 7.1 `ManualProjector`

Works entirely on `JointsWorld` (Z-up millimetres):

```
forward = normalize(centroid - cameraPos)
right   = normalize(cross(forward, worldUp))     // worldUp = (0, 0, 1)
camUp   = cross(right, forward)

d = joint - cameraPos
u = focal * dot(d, right) / dot(d, forward) + 500
v = 500 - focal * dot(d, camUp) / dot(d, forward)
```

`v` is subtracted because image rows increase downward while `camUp` points up.

### 7.2 `GodotProjector`

```csharp
return challengeCam.UnprojectPosition(jointNodes[jointIndex].GlobalPosition);
```

This projector is constructed with references to `ChallengeCam` and `PoseFigure` and reads the
live scene graph, so its `Begin` only records the frame index; the frame and focal length reach
it through the scene, which `Main` has already updated. `ManualProjector.Begin`, by contrast,
does real work: it builds the basis in §7.1 from the raw data.

### 7.3 Evidence they agree

Both were run against frame 00 during design, alongside an independent Python implementation:

| Joint | Godot `unproject_position` | Python pinhole |
| --- | --- | --- |
| Hip | 502.33258, 517.53894 | 502.333, 517.539 |
| Neck | 500.29883, 422.88983 | 500.299, 422.890 |
| RAnkle | 428.33932, 577.69910 | 428.339, 577.699 |

Agreement to four decimals. This is expected rather than lucky: Godot's perspective projection
with vertical FOV θ over a viewport of height H gives `v = H/2 · (1 − (Y/Z)/tan(θ/2))`, and with
`tan(θ/2) = 500/f` and H = 1000 that is exactly `500 − f·Y/Z`.

## 8. Scene and modes

One 3D scene holds the floor grid, the figure, a marker at the challenge camera position, and a
line from that marker to the subject. Two cameras live in it: `FlyCamera` and `ChallengeCam`.
A `CanvasLayer` carries the overlay and the HUD.

**3D mode** — `FlyCamera.Current = true`, scene rendered normally, mouse look active.

**2D mode** — `ChallengeCam.Current = true`, mouse always free, and `ProjectionOverlay` paints an
opaque background across the whole viewport before drawing the bones on top. The 3D render is
therefore hidden behind the overlay rather than needing to be disabled. The background is either
white (task b) or `frames/NN.png` (task d).

Bones are drawn with `DrawLine` and joints with `DrawCircle`, at the coordinates returned by the
active projector.

### 8.1 View framing

Projected into true image pixels, the figure is small: measured across all 20 frames it spans
only 88–207 px horizontally and 97–289 px vertically inside the 1000×1000 canvas, always well
inside the frame (u ∈ [404.9, 612.3], v ∈ [381.6, 671.0]). On the photograph that is correct and
necessary. On a white background it would be an unreadable blob in the middle, and would *not*
resemble the right-hand panel of `pose-sample.png`, which is scaled to the pose.

The overlay therefore has two framings:

```csharp
public enum ViewFraming { ImagePixels, FitToPose }
```

- **`ImagePixels`** draws at (u, v) directly. Required whenever the photograph is the background,
  since anything else destroys registration.
- **`FitToPose`** computes the bounding box of the frame's projected points, pads it by 10%, and
  applies a uniform scale and translation so it fills the viewport. Display only — it never
  changes the numbers, which stay in image pixels everywhere they are recorded.

Framing is chosen in the control panel. Selecting the photograph background forces
`ImagePixels` and disables the framing control.

### 8.2 Keyboard and mouse

The keyboard drives the camera and nothing else. Every other operation is a control in the
panel (§8.3) — there are no shortcut keys for them.

| Input | Action |
| --- | --- |
| `W` `A` `S` `D` | move the fly camera |
| `Q` / `E` | move down / up |
| `Shift` | move faster |
| mouse | look, while the cursor is captured |
| `Esc` | release the cursor |

Cursor handling: clicking inside the viewport in 3D mode captures the cursor for mouse look;
`Esc` releases it so the panel can be clicked. `WASD`/`Q`/`E` keep working either way, so you can
fly with the cursor free. In 2D mode the cursor is never captured — there is nothing to look
around — and clicks go to the panel.

### 8.3 Control panel

A `PanelContainer` holding a `VBoxContainer`, docked to the top-left of a `CanvasLayer` above the
overlay, visible in both modes:

| Control | Type | Purpose |
| --- | --- | --- |
| Frame | `HSlider` (0–19) + `◀` / `▶` `Button`s + `Label` `07 / 20` | step or scrub through the frames |
| View | `OptionButton`: 3D scene / 2D projection | replaces the mode toggle |
| Projection method | `OptionButton`: Manual pinhole / Godot unproject | switches the active projector live |
| Framing | `OptionButton`: Fit to pose / True image pixels | disabled while the background is the photograph |
| Background | `OptionButton`: White / Photograph | 2D only |
| Snap to challenge camera | `Button` | moves the fly camera to the challenge camera's position, orientation and FOV |
| Export | `Button` | runs §9; shows progress and the output path when done |
| Method agreement | `Label` | max \|Δ\| in pixels between the two methods for the current frame |

The agreement label means the two implementations can be watched agreeing while the tool is in
use, not only at export time.

`Main` carries `[Export] ProjectionMethod DefaultMethod` so the initial choice is also settable
from the editor Inspector.

The panel is hidden for the duration of each PNG capture (§9) so it never appears in an exported
image, and restored afterwards.

## 9. Export (tasks b, c, d)

Triggered by the Export button. All CSV writing goes through CsvHelper. Output lands in `res://out/`, which is
gitignored:

| File | Shape | Purpose |
| --- | --- | --- |
| `coords_wide.csv` | 20 rows × 29 cols: `frame, u0, v0 … u13, v13` | **the deliverable table** (task c), from the active method; mirrors `poses.txt`'s layout |
| `coords_long.csv` | 560 rows: `frame, joint_id, joint_name, method, u, v` | readable form, both methods |
| `method_agreement.csv` | 20 rows: `frame, max_abs_diff_px, mean_abs_diff_px` | evidence the two methods match |
| `proj_00.png` … `proj_19.png` | 1000×1000 | task b — skeleton on white, **`FitToPose`** framing, to resemble the sample's right panel |
| `overlay_00.png` … `overlay_19.png` | 1000×1000 | task d — skeleton on the photograph, **`ImagePixels`** framing |

Images are captured by hiding the control panel, switching to 2D mode, setting the frame,
background and framing, awaiting
`ToSignal(RenderingServer, RenderingServer.SignalName.FramePostDraw)`, then
`GetViewport().GetTexture().GetImage().SavePng(path)`.

A `--export-csv` command-line flag runs the CSV half and quits. This half works under
`--headless`; PNG capture does not, because headless has no renderer.

## 10. Verification

1. **Three-way numeric check.** `tools/check_projection.sh` runs
   `godot-mono --headless --path . -- --export-csv`, then `tools/reference_projection.py`
   recomputes all 280 points independently in Python and diffs against both methods in
   `coords_long.csv`. Any deviation above 0.01 px fails.
2. **Range check.** Every projected point for all 20 frames must land inside [0, 1000] in both
   axes; a joint outside the frame would mean the orientation is wrong. Measured during design,
   the true bounds are u ∈ [404.9, 612.3] and v ∈ [381.6, 671.0], so the test has ample margin
   and will not fail spuriously.
3. **Visual check.** `overlay_00.png` is compared against `frames/00.png` — the skeleton should
   sit on the seated figure. Exact registration is not expected, and the challenge text says so.
4. **Shape check.** `coords_wide.csv` has 20 data rows and 29 columns.

## 11. Code layout

```
project.godot
PoseProjection.csproj
.gitignore                     out/, .godot/, bin/, obj/
README.md                      what it is, how to run, the answers to (a)-(d)
data/
  focal.txt  joint-names.txt  poses.txt
  frames/00.png … 19.png
scenes/
  Main.tscn
scripts/
  PoseFrame.cs           CameraWorld/Godot, JointsWorld[14], JointsGodot[14], centroid
  PoseData.cs            parse the three files; hold the 20 PoseFrames
  Bones.cs               the 13 edges and their colours
  IProjector.cs          the interface and the ProjectionMethod enum
  ManualProjector.cs     hand-rolled pinhole
  GodotProjector.cs      unproject_position
  Framing.cs             FitToPose / ImagePixels transform for the 2D view
  CoordinateTable.cs     the wide, long and agreement CSVs
  PoseLoader.cs          reads the res:// files and hands readers to PoseData
  PoseFigure.cs          builds and repositions the 3D joint/bone meshes
  FlyCamera.cs           WASD + mouse look
  ProjectionOverlay.cs   the 2D drawing
  ControlPanel.cs        every non-camera operation
  Exporter.cs            CSV and PNG output
  SelfTest.cs            headless comparison of the two projectors
  Main.cs                owns frame index and mode; wires everything together
tests/
  .gdignore              keeps the Godot editor out of the test project
  PoseProjection.Tests/  xUnit over the engine-free scripts above
tools/
  reference_projection.py
  check_projection.sh
out/                     gitignored
```

The `data/` directory is committed (~18 MB) so the project runs straight after a clone.

Class names avoid collisions with Godot's own (`Skeleton3D`, `Camera3D`), hence `PoseFigure` and
`FlyCamera`.

The split above is load-bearing for testing. Everything from `PoseFrame.cs` down to
`CoordinateTable.cs` is a plain class with no `Node` base and no scene-graph access, so the test
project compiles those sources directly and runs them under xUnit outside the engine. The
remaining files are engine-bound and are covered by the headless self-test instead.

## 12. Style

Plain, ordinary C#: `for` loops rather than LINQ, explicit types rather than `var` chains, no
records, no pattern matching, no expression-bodied members. Comments only where the geometry is
not self-evident — the axis conversion, the `v` sign flip, the FOV derivation. Files stay small
and single-purpose.

## 13. Risks

- **PNG capture timing.** If a captured frame ever comes out blank or one frame stale, wait two
  `FramePostDraw` signals instead of one. Cheap to fix, worth watching for.
- **`res://out/` writability.** Fine when running from source, which is how this tool is used. If
  it ever fails, fall back to `user://` and print the globalized path.
- **CsvHelper float formatting.** Write coordinates with an explicit invariant-culture format so
  the table is stable across locales.
