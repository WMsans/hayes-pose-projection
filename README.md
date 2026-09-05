# Hayes Pose Projection

An answer to the challenge on the
[Human Pose Estimation via AI/ML](https://ics.uci.edu/~wayne/research/students/)
project, built as an interactive Godot 4 tool in C#.

## Running it

Requires the .NET build of Godot 4.7 (`godot-mono`) and the .NET SDK.

    dotnet build PoseProjection.csproj
    godot-mono --path .

Move the camera with `W A S D`, `Q`/`E` for down and up, `Shift` to go faster.
Click in the window to look around with the mouse; `Esc` frees the cursor.
Everything else is in the control panel: stepping through the 20 frames,
switching between the 3D scene and the 2D projection, choosing the projection
method, the framing and the background, snapping the free camera to the
challenge camera, and exporting.

## The answers

**(a) Camera orientation.** For each frame the camera sits at the position
given in the first three columns of `poses.txt` and looks at the centroid of
that frame's 14 joints, with no roll. The centroid is used rather than the hip
so that crouched and reaching poses stay framed.

**(b) The 2D projection.** A pinhole model with focal length 1148.6 and the
principal point at the centre of the 1000x1000 frame. `focal.txt` is labelled
millimetres, but the value is in pixels: this is Human3.6M, whose cameras have
f of about 1145 px, and no sensor size is supplied that would let a millimetre
figure be used. The resulting field of view, 47.048 degrees, matches the frames.

**(c) The table.** `out/coords_wide.csv` holds all 280 coordinates, one row per
frame and 28 columns of `u0,v0 ... u13,v13`, mirroring the layout of
`poses.txt`. `out/coords_long.csv` is the same data one row per joint, and
includes both projection methods.

**(d) Superimposed on the frames.** `out/overlay_00.png` through
`overlay_19.png`. As the challenge notes, the reconstructed viewpoint is not
identical to the photograph's, so the fit is close but not exact.

## Two projection methods

The projection is implemented twice and either can be selected in the control
panel:

- **Manual pinhole** builds the camera basis by hand from the raw millimetre
  data and applies `u = f * x / z + 500` directly. It never touches the scene
  graph.
- **Godot unproject** places a `Camera3D` at the same position with a field of
  view derived from the focal length and calls `unproject_position`.

They agree to within 0.0001 px, which is not a coincidence: Godot's perspective
projection over a viewport of height H with vertical FOV theta gives
`v = H/2 * (1 - (y/z) / tan(theta/2))`, and with `tan(theta/2) = 500/f` and
H = 1000 that is exactly `500 - f * y / z`. The panel shows the live difference
between them, and `out/method_agreement.csv` records it per frame.

The viewport is locked to 1000x1000 in `project.godot`. This matters: without
it, `unproject_position` would return coordinates that depend on the window
size, and the exported table would not be verifiable against the frames.

## Checks

    ./tools/check_projection.sh

Runs the unit tests, an in-engine headless comparison of the two projectors
across all 280 points, and `tools/reference_projection.py`, a third
implementation in plain Python that recomputes every coordinate from the raw
data and diffs it against the exported table.

## Layout

    data/         the contents of Pose.zip
    scripts/      the C# source
    scenes/       the Godot scene
    tests/        xUnit tests for the engine-free classes
    tools/        the Python reference and the check script
    out/          exported tables and images (not committed)
