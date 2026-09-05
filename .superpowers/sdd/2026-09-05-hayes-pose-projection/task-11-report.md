# Task 11 Report: Export the tables and images

## Status

Implemented Task 11 on top of Tasks 1–10. No subagents were dispatched.

## Files

- Created `scripts/Exporter.cs`
  - Added `Exporter(Main)`, `OutputDirectory()`, `WriteTables()`, and asynchronous `WriteImages()`.
  - Exports active coordinates plus manual/Godot comparison tables.
  - Captures fit-to-pose white-background images and image-pixel photograph overlays.
  - Hides the control panel during capture and restores overlay, frame, view, and panel state afterward.
- Modified `scripts/Main.cs`
  - Replaced the export stub with the table/image export flow and final status message.
  - Added the `--export-csv` command-line branch immediately below `--self-test`.
- Created `.superpowers/sdd/2026-09-05-hayes-pose-projection/task-11-report.md` (this report).
- Preserved `.gitignore`, including `out/`; generated local Godot `.import`/`.uid` files were removed after verification.

## Verification

### Test-first red check

Before implementation, the requested headless export behavior was run with a Python timeout wrapper because this environment has no `timeout` utility:

```bash
python3 - <<'PY'
import subprocess
try:
    result = subprocess.run(
        ['godot-mono', '--headless', '--path', '.', '--', '--export-csv'],
        text=True, capture_output=True, timeout=8)
    print('exit=' + str(result.returncode))
except subprocess.TimeoutExpired:
    print('timeout (expected before --export-csv branch)')
PY
```

It timed out after normal application startup, before any CSV output, as expected while the command-line branch was absent. The initial run also exposed the fresh worktree's missing image-import metadata; the editor import step below resolved that runtime prerequisite.

### Unit tests

```bash
dotnet test tests/PoseProjection.Tests/PoseProjection.Tests.csproj --nologo
```

Output:

```text
Passed!  - Failed:     0, Passed:    33, Skipped:     0, Total:    33
```

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

### Godot asset import prerequisite

```bash
godot-mono --editor --headless --path . --quit
```

Completed with exit code 0 and imported the 20 frame PNGs. Generated `.import` and `.uid` metadata was removed afterward because it is local editor state.

### Headless Godot Mono self-test

```bash
godot-mono --headless --path . -- --self-test
```

Output:

```text
self-test: OK, both projectors agree on all 280 points
exit=0
```

### Headless CSV export and shape/max-diff checks

```bash
godot-mono --headless --path . -- --export-csv
wc -l out/coords_wide.csv out/coords_long.csv out/method_agreement.csv
head -1 out/coords_wide.csv | tr ',' '\n' | wc -l
cut -d, -f2 out/method_agreement.csv | tail -n +2 | sort -g | tail -1
```

Output:

```text
wrote tables to .../out
     21 out/coords_wide.csv
    561 out/coords_long.csv
     21 out/method_agreement.csv
    603 total
     29
0.000183
```

The required counts are 21, 561, and 21; the wide header has 29 fields; and the largest method difference is under 0.01 px.

### Interactive/image smoke check

Not available in this environment: `DISPLAY` and `WAYLAND_DISPLAY` are unset, and no GUI automation package is installed. Consequently, the button-driven image export and PNG pixel inspection were not run. The source review confirms the exact requested two-frame redraw wait, panel hiding, fit/image-pixel modes, photograph selection, and state restoration paths.

### Final hygiene

```bash
git diff --check
```

Completed cleanly. Before staging, `git status` contained only `scripts/Main.cs` and `scripts/Exporter.cs`; `out/` remained ignored and generated editor metadata was absent.

## Self-review

- [x] `Exporter` API and implementation match the Task 11 brief.
- [x] `WriteTables` records manual, Godot, and active methods for every frame, restores the selected frame, and writes all three required CSVs.
- [x] Coordinates remain true image pixels because export uses `ProjectCurrent` directly; only image drawing applies fit-to-pose framing.
- [x] `WriteImages` hides the panel, enables 2D mode, captures both requested image sets, waits for two `FramePostDraw` signals, and restores state.
- [x] `RunExport` writes tables, awaits images, refreshes panel readouts, and returns the specified status text.
- [x] `--export-csv` is directly below `--self-test`, performs only headless-safe table work, prints the required status, and quits with code 0.
- [x] Existing UI, overlay, camera, frame, and `.gitignore` behavior was not otherwise changed.

## Concerns

No code concerns found. The only limitations are environmental: the shell lacks `timeout`, the fresh worktree initially required a Godot editor import for PNG loading, and no graphical display was available for interactive image inspection. The final clean headless import/self-test/export sequence passed.

## Commit

`Export the coordinate tables and the projection and overlay images`

## Fix report: round 1

### Status

Fixed the review findings in `scripts/Exporter.cs`. `WriteImages` now saves the panel's prior visibility and restores it, along with the overlay mode, photograph flag, frame, and view mode, from a `finally` block when capture or PNG writing fails.

### Covering regression check

- `test_write_images_restores_panel_visibility_and_state_on_failure_path`
- Red before the fix: `python3 /tmp/test_exporter_state_restoration.py` failed because the panel visibility snapshot was absent.
- Green after the fix: same command printed `PASS: exporter state-restoration regression check`.

### Verification

```text
$ dotnet build PoseProjection.csproj --nologo
Build succeeded. 0 Warning(s), 0 Error(s)

$ dotnet test tests/PoseProjection.Tests/PoseProjection.Tests.csproj --nologo
Passed! - Failed: 0, Passed: 33, Skipped: 0, Total: 33

$ godot-mono --editor --headless --path . --quit
exit 0; imported the frame PNG assets (generated import metadata removed before commit)

$ godot-mono --headless --path . -- --self-test
self-test: OK, both projectors agree on all 280 points

$ godot-mono --headless --path . -- --export-csv
wrote tables to .../out
21 out/coords_wide.csv
561 out/coords_long.csv
21 out/method_agreement.csv
29
0.000183

$ git diff --check
clean
```

The CSV checks match the required 21/561/21 row counts, 29 wide-header fields, and less than 0.01 px maximum method difference.

### Self-review

- Confirmed the temporary image-export state is captured before mutation.
- Confirmed cleanup is in `finally` and restores the exact prior panel visibility rather than forcing visibility on.
- Confirmed the required table, build, unit-test, self-test, and headless CSV behavior remains unchanged.

### Concerns

Interactive image capture and an injected PNG-write failure were not exercised because this environment has no graphical display or GUI automation. The focused regression check verifies the required save/`finally` structure; headless build, unit, self-test, and CSV checks passed.
