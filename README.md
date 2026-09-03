# 3D-to-2D Human Pose Projection

Solution to Wayne Hayes' UCI take-home research challenge
(<https://ics.uci.edu/~wayne/research/students/>): project 20 supplied 3D
skeletons onto 2D images from the supplied camera positions.

## Build

```bash
./scripts/fetch-data.sh          # downloads Pose.zip into data/ (not committed)
cmake -S . -B build && cmake --build build -j
./build/pose_tests               # unit tests
```

## Run

```bash
./build/pose-project --data data/Pose --out out --mode both
./build/pose-explorer --data data/Pose     # optional debug tool; needs a display
```

## Credits

Camera calibration from
[karfly/human36m-camera-parameters](https://github.com/karfly/human36m-camera-parameters)
(MIT, © 2019 Karim Iskakov), vendored in `third_party/h36m/`.
