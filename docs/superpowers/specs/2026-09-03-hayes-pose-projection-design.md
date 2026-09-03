# 3D-to-2D Human Pose Projection — Design

Date: 2026-09-03
Challenge: Wayne Hayes (UCI), "Human Pose Estimation: 3D-to-2D Projection"
Source: <https://ics.uci.edu/~wayne/research/students/>

## 1. The task as stated

From the challenge page, given `Pose.zip` (20 frames, `poses.txt`, `focal.txt`,
`joint-names.txt`), write a program that:

- **(a)** finds a suitable camera orientation that points to the subject from the camera
  position given in the first 3 columns of `poses.txt`;
- **(b)** projects the 3D skeleton onto a 2D image, rendered on a white background rather
  than overlaid on the photograph;
- accumulates every 2D coordinate used into **one table**, so the results can be verified;
- **(c)** *optional, explicitly "not required"*: superimposes the skeleton onto the frames.

The graded deliverable is a **PDF write-up with figures**. The challenge page states the
write-up is "just as much a test of your communication skills as coding," and that the work
must be done independently, citing any references used.

Scope decision: implement (a), (b), the table, **and** (c). The overlay is the only way to
verify the projection is correct rather than merely self-consistent.

## 2. What the data actually is

Established by inspection before any code was written:

| Fact | Value |
| --- | --- |
| Frames | 20 PNG, 1000×1000, RGB |
| `poses.txt` | 20 rows × 45 columns |
| Column layout | cols 1–3 = camera position (mm); cols 4–45 = 14 joints × (X, Y, Z) |
| Joints | 14, named in `joint-names.txt` (Hip, RHip, RKnee, RAnkle, LHip, LKnee, LAnkle, Neck, LUpperArm, LElbow, LWrist, RUpperArm, RElbow, RWrist) |
| `focal.txt` | 1148.6 (pixels) |
| Distinct camera positions | **4**, five frames each |

The 20 rows use only four distinct camera positions:

| Camera position (mm) | Rows (0-indexed) |
| --- | --- |
| (1761.28, −5078.01, 1606.26) | 0, 2, 5, 7, 9 |
| (−1846.78, 5215.05, 1491.97) | 1, 3, 16, 18, 19 |
| (1841.11, 4955.28, 1563.45) | 4, 12, 13, 14, 17 |
| (−1794.79, −3722.70, 1574.89) | 6, 8, 10, 11, 15 |

This is **Human3.6M, Subject S1**, cameras 55011271, 58860488, 54138969 and 60457274
respectively. Verified by computing camera centres `C = −Rᵀt` from the published H36M
calibration and matching against the file: agreement is exact to **0.0000 mm** for all four.

Joint coordinates are true world coordinates in millimetres (not root-relative): the subject
happens to sit near the H36M world origin, which is the centre of the capture room.

## 3. Prior art and reuse

A search found **no public solution to this challenge**, so all projection, rendering and
analysis code is written from scratch. One external data asset is reused:

- [karfly/human36m-camera-parameters](https://github.com/karfly/human36m-camera-parameters)
  (MIT, © 2019 Karim Iskakov) — published H36M intrinsics (calibration matrix, distortion)
  and extrinsics (R, t) for all subjects and cameras. Vendored as a JSON file with its
  licence and attribution retained, and cited in the write-up.

Libraries, all pulled by CMake `FetchContent` so nothing is installed system-wide:

| Library | Purpose |
| --- | --- |
| raylib | offscreen rendering, PNG export, and the interactive explorer |
| raygui | explorer UI widgets (sliders, toggles, frame scrubber) |
| glm | vector/matrix math |
| nlohmann/json | reading the vendored calibration |
| doctest | unit tests |

## 4. The central finding the design is built around

Part (a) is under-determined. A camera position alone fixes only the camera's *location*;
the orientation still has a free target choice and a free roll. Any "suitable" orientation is
therefore a **choice that must be justified**, and the natural choice — look-at the subject,
world +Z as up — has a consequence that can be measured rather than argued:

> A look-at rotation aimed at the joint centroid places that centroid exactly at the
> principal point of the image, by construction. The real cameras do not.

Measured against the true calibration:

| Row | Camera | Centroid, true extrinsics | Centroid, look-at |
| --- | --- | --- | --- |
| 0 | 55011271 | (559.3, 547.1) | (499.8, 500.5) |
| 1 | 58860488 | (679.1, 443.8) | (499.9, 500.0) |
| 4 | 54138969 | (449.2, 579.1) | (500.3, 500.0) |
| 6 | 60457274 | (560.6, 537.9) | (499.9, 499.6) |

The true projection for row 0 lands at (559, 547) — where the subject actually sits in
`frames/00.png`, confirming both the calibration match and the world-coordinate reading.
Look-at is off by up to ~180 px. This is a systematic, explainable error, not a bug, and
quantifying it is the analytical spine of the write-up.

A second, smaller finding: `focal.txt`'s 1148.6 is exactly camera 55011271's focal length.
The other three cameras differ (1144.4–1149.0), so the single supplied focal is only strictly
correct for one quarter of the frames.

Consequently the program implements **both** projection modes and compares them:

- `lookat` — the challenge as literally specified; self-contained, no external data.
- `gt` — published calibration; used for the overlay and as the reference for error analysis.

## 5. Architecture

Eight units, each with one purpose and a testable interface. Two binaries are built:
`pose-project` (batch, produces the deliverable) and `pose-explorer` (interactive, optional).

### `pose_io`
Parses `poses.txt`, `joint-names.txt`, `focal.txt` into a `Frame { vec3 camera_position;
array<vec3,14> joints; }`. Pure functions, no I/O policy beyond reading the given paths.
Rejects malformed input loudly (wrong column count, non-finite values).

### `camera`
- `look_at(eye, target, world_up) -> mat3` — forward = normalize(target − eye), right =
  normalize(forward × world_up), up = right × forward. Right-handed, re-orthonormalized.
- `load_calibration(path) -> map<CameraId, Calibration>` — vendored H36M JSON.
- `identify(camera_position, calibration, tol_mm) -> optional<CameraId>` — nearest published
  camera centre within tolerance.
- `project(point3, rotation, translation, intrinsics, apply_distortion) -> vec2`.
  Pinhole: `u = f·X/Z + cx`, `v = f·Y/Z + cy`. Radial/tangential distortion optional, off for
  `lookat` (no distortion model is knowable from the challenge data alone), on for `gt`.

### `skeleton`
The 14-joint bone topology (parent list) and left/right/torso limb colouring. Data only.

### `draw`
Drawing primitives shared by the batch renderer and the interactive explorer: skeleton in 2D
(bones, joints, optional labels), skeleton in 3D, camera frustum, floor grid. Takes an
abstract target so the same call sequence runs against a `RenderTexture` (batch) and the
screen (explorer). **This sharing is a correctness property, not just tidiness: the figures in
the PDF are produced by the same code path the explorer displays.**

### `render`
Batch, raylib offscreen via a hidden window plus `RenderTexture`:
- `white/NN.png` — 1000×1000, skeleton on white, joints as dots, bones as coloured lines.
- `overlay/NN.png` — the same skeleton composited onto `frames/NN.png`.
- `panel/NN.png` — overlay beside white render, echoing the challenge page's `pose-sample.png`.

### `explorer` (separate binary, `pose-explorer`)
A deliberately plain debug tool for inspecting the data and grabbing report figures. Not
required by the challenge, and kept small on purpose: **one full-window view at a time, plus a
raygui debug panel of buttons, text and sliders.** No split screen, no custom UI work, no
gameplay.

- **View (one at a time, switched by a button):**
  - *2D* (default) — the projected skeleton, drawn over the real frame or a white background.
  - *3D* — a bare orbit camera over the capture volume: floor grid, the frame's skeleton, and
    the look-at and ground-truth frusta of the active camera drawn together, so §4's
    orientation discrepancy is visible as two diverging cones. This is the one figure the
    write-up cannot easily make any other way, which is why the 3D view survives the cut.
- **Debug panel:** frame slider (0–19); buttons for projection mode (`lookat` / `gt` / both),
  background (photo / white), distortion on/off, view (2D / 3D), and figure export; a slider
  for photo opacity; text lines for the active H36M camera id and the mean and worst per-joint
  reprojection error of the current frame.

The explorer is read-only with respect to the deliverable: it cannot change the numbers in
the coordinate table, only display them.

## 6. Outputs

```
out/
  coords/all-2d-coordinates.csv     # the single consolidated table (row, joint, u, v, mode)
  coords/all-2d-coordinates.tex     # same, as a LaTeX table for the report
  white/00.png … 19.png             # required part (b)
  overlay/00.png … 19.png           # optional part (c)
  panel/00.png … 19.png             # side-by-side figures for the report
  analysis/joint-error.csv
  analysis/camera-angular-error.csv
  figures/                          # written by pose-explorer's figure-export key
report/report.tex                   # \input's the generated tables, \includegraphics the figures
report/report.pdf                   # built by pdflatex; the graded deliverable
```

The report is a build target, not a hand-assembled document: regenerating figures and
rebuilding the PDF is one command.

## 7. Error handling

| Condition | Behaviour |
| --- | --- |
| Malformed `poses.txt` (column count, non-finite) | fail with the offending line number |
| Joint with camera-space z ≤ 0 (behind camera) | reported per frame in the analysis output, not silently clipped |
| Missing `frames/NN.png` | overlay for that row skipped with a warning; white render still produced |
| Camera position matches no published centre within tolerance | degrade to `lookat` only, warn; never emit a `gt` overlay from a guessed camera |
| No GL context available (batch) | documented `xvfb-run` fallback; failure message names it |
| No display available (explorer) | refuses to start with a message pointing at the batch tool; the deliverable never depends on the explorer |

## 8. Testing

TDD, doctest, no network in tests.

- `look_at` returns an orthonormal right-handed basis for arbitrary eye/target pairs.
- A point at the look-at target projects to the principal point (the property that produces
  the §4 finding).
- A synthetic camera with a hand-computed R, t and K maps a known 3D point to a known pixel.
- Projection is invariant to the units of the camera basis (scale-independence).
- `pose_io` round-trips a fixture file; rejects a truncated row.
- `identify` returns the right camera at 0 mm, and `nullopt` past tolerance.
- Limb lengths are preserved in 3D and vary smoothly under projection.

Rendering is not unit-tested. It is validated by the overlay landing on the subject and by
the numeric error tables — both of which appear as evidence in the write-up. The explorer is
not unit-tested either; the logic it displays lives in `camera` and `analysis`, which are, and
its drawing goes through the same `draw` unit the batch figures use.

## 9. Write-up outline (the actual deliverable)

1. Problem, data, and what the 45 columns are.
2. Identifying the four cameras from the data (the clustering observation).
3. Part (a): constructing the look-at orientation, and why the free roll must be chosen.
4. Part (b): the projection model, and all 20 white-background renders.
5. The consolidated 2D coordinate table.
6. Part (c): overlays on the real frames, as verification.
7. Error analysis: look-at vs. published calibration, per joint and per camera; the
   principal-point artefact; the single-focal-length issue.
8. Limitations, and references (H36M, the calibration repository, each library).

An appendix documents the interactive explorer with a screenshot: it is not part of the
challenge, but it is how several of the report's figures were produced and it demonstrates the
orientation ambiguity of §4 visually.

## 10. Open items

- `texlive` (pdflatex + recommended packages) must be installed locally before the PDF builds.
- The explorer needs a working GL context; on this machine raylib and raygui are fetched by
  CMake, but no system raylib is installed, so the first build will download and compile them.
- Repository visibility: created private. A challenge solution left public before submission
  could hand answers to other applicants; flip with `gh repo edit --visibility public` if the
  repo is wanted as a portfolio link.
