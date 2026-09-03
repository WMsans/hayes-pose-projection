# Hayes 3D-to-2D Pose Projection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a C++ program that projects the 20 supplied 3D human skeletons onto 2D images from the supplied camera positions, emits one consolidated coordinate table, overlays the result on the real photographs as verification, and produces a LaTeX PDF write-up — plus a small raylib debug explorer.

**Architecture:** Eight small units (`pose_io`, `skeleton`, `camera`, `analysis`, `draw`, `render`, and two `main`s) behind a static core library `pose_core`, consumed by two binaries: `pose-project` (batch, produces the graded deliverable) and `pose-explorer` (interactive debug tool, optional, never part of the pipeline). Two projection modes exist side by side: `lookat`, constructed only from data the challenge supplies, and `gt`, using published Human3.6M calibration; the measured gap between them is the analytical content of the write-up.

**Tech Stack:** C++20, CMake ≥ 3.20 with `FetchContent` (raylib 5.5, raygui 4.0, glm 1.0.1, nlohmann/json 3.11.3, doctest 2.4.11 — nothing installed system-wide except OS graphics headers and `texlive`), LaTeX for the report.

**Spec:** `docs/superpowers/specs/2026-09-03-hayes-pose-projection-design.md`

## Global Constraints

- Language: C++20. Warnings on: `-Wall -Wextra`. No compiler-specific extensions.
- All third-party libraries are fetched by CMake `FetchContent` with pinned tags. Do not add a system-package dependency other than OS graphics/X11/Wayland headers required by raylib.
- Units are **millimetres** throughout for 3D, **pixels** for 2D. Never mix.
- Images are **1000 × 1000**. `focal.txt` = **1148.6** px. **14** joints per pose. `poses.txt` = **20 rows × 45 columns** (cols 1–3 camera position, cols 4–45 = 14 × XYZ).
- Ground-truth data is Human3.6M **Subject S1**; the four camera ids are `55011271`, `58860488`, `54138969`, `60457274`.
- The batch pipeline must never require a display or the explorer; `pose-project` must run headless (with `xvfb-run` if no GL context exists).
- Outputs must be deterministic: identical inputs produce byte-identical CSV files.
- **Never commit the challenge data** (`Pose/`, frames, `poses.txt`) or generated output. Frames are Human3.6M imagery and are not ours to redistribute.
- Attribution: the vendored calibration JSON keeps its MIT licence and author credit, and is cited in the report.
- Commit after every task. Commit messages use Conventional Commits (`feat:`, `test:`, `docs:`, `chore:`).

---

## File Structure

| File | Responsibility |
| --- | --- |
| `CMakeLists.txt` | project, C++20, targets `pose_core`, `pose-project`, `pose-explorer`, `pose_tests` |
| `cmake/Dependencies.cmake` | all `FetchContent` declarations, pinned |
| `src/pose_io.hpp/.cpp` | parse `poses.txt`, `focal.txt`, `joint-names.txt` |
| `src/skeleton.hpp/.cpp` | 14-joint bone topology and limb colouring |
| `src/camera.hpp/.cpp` | look-at construction, calibration loading, camera identification, projection |
| `src/analysis.hpp/.cpp` | per-joint pixel error, per-camera angular error, limb lengths, CSV writing |
| `src/draw.hpp/.cpp` | shared drawing primitives (2D skeleton, 3D skeleton, frustum, grid) |
| `src/render.hpp/.cpp` | offscreen batch rendering to PNG |
| `src/main_project.cpp` | `pose-project` CLI |
| `src/main_explorer.cpp` | `pose-explorer` debug tool |
| `tests/*.cpp` | doctest unit tests, one file per unit |
| `tests/fixtures/mini_poses.txt` | 2-row synthetic pose file |
| `third_party/h36m/camera-parameters.json` | vendored H36M calibration (MIT) |
| `third_party/h36m/LICENSE` | upstream licence text |
| `scripts/fetch-data.sh` | downloads and unpacks `Pose.zip` into `data/` |
| `report/report.tex` | the graded write-up |
| `README.md` | build, run, reproduce |

---

### Task 1: Project scaffold, dependencies, and a passing test binary

**Files:**
- Create: `CMakeLists.txt`, `cmake/Dependencies.cmake`, `.gitignore`, `README.md`, `scripts/fetch-data.sh`, `third_party/h36m/camera-parameters.json`, `third_party/h36m/LICENSE`
- Test: `tests/test_smoke.cpp`

**Interfaces:**
- Consumes: nothing.
- Produces: CMake targets `pose_core` (static lib), `pose_tests` (doctest runner). Build dir convention is `build/`; the test binary is `build/pose_tests`.

- [ ] **Step 1: Verify OS graphics headers exist (raylib needs them)**

Run:
```bash
pkg-config --exists x11 && echo "x11 ok" || echo "MISSING x11"
ls /usr/include/GL/gl.h && echo "gl ok" || echo "MISSING mesa headers"
```
Expected: both `ok`. If missing on Arch: `sudo pacman -S --needed libx11 libxrandr libxinerama libxcursor libxi mesa`.

- [ ] **Step 2: Write `.gitignore`**

```gitignore
build/
data/
out/
report/*.pdf
report/*.aux
report/*.log
report/*.out
```

- [ ] **Step 3: Write `cmake/Dependencies.cmake`**

```cmake
include(FetchContent)

# Several pinned dependencies declare cmake_minimum_required below 3.5,
# which CMake 4.x rejects outright. This floor keeps them configurable.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # raylib
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)   # raylib
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)  # nlohmann

FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5 GIT_SHALLOW TRUE)
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3 GIT_SHALLOW TRUE)
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11 GIT_SHALLOW TRUE)
# raygui has no CMakeLists.txt at its root, so MakeAvailable only populates it;
# we add ${raygui_SOURCE_DIR}/src to the include path by hand.
FetchContent_Declare(raygui
  GIT_REPOSITORY https://github.com/raysan5/raygui.git
  GIT_TAG 4.0 GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(raylib glm nlohmann_json doctest raygui)
```

- [ ] **Step 4: Write the top-level `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20)
project(hayes_pose_projection LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

include(cmake/Dependencies.cmake)

add_library(pose_core STATIC)
target_sources(pose_core PRIVATE src/placeholder.cpp)
target_include_directories(pose_core PUBLIC src ${raygui_SOURCE_DIR}/src)
target_link_libraries(pose_core PUBLIC glm::glm nlohmann_json::nlohmann_json raylib)

enable_testing()
add_executable(pose_tests tests/test_smoke.cpp)
target_link_libraries(pose_tests PRIVATE pose_core doctest::doctest)
add_test(NAME unit COMMAND pose_tests)
```

Also create `src/placeholder.cpp` containing exactly:

```cpp
// Replaced in Task 2 by the first real translation unit.
namespace pose { int link_anchor() { return 0; } }
```

- [ ] **Step 5: Write the smoke test**

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

namespace pose { int link_anchor(); }

TEST_CASE("core library links") { CHECK(pose::link_anchor() == 0); }
```

- [ ] **Step 6: Configure and build**

Run: `cmake -S . -B build && cmake --build build -j`
Expected: configures and builds. First run downloads raylib and takes several minutes. If raylib fails on missing X11/Wayland headers, install the packages from Step 1 and re-run.

- [ ] **Step 7: Run the test**

Run: `./build/pose_tests`
Expected: `[doctest] assertions: 1 | 1 passed | 0 failed`

- [ ] **Step 8: Vendor the H36M calibration**

Run:
```bash
mkdir -p third_party/h36m
curl -sL -o third_party/h36m/camera-parameters.json \
  https://raw.githubusercontent.com/karfly/human36m-camera-parameters/master/camera-parameters.json
curl -sL -o third_party/h36m/LICENSE \
  https://raw.githubusercontent.com/karfly/human36m-camera-parameters/master/LICENSE
python3 -c "import json;d=json.load(open('third_party/h36m/camera-parameters.json'));print(sorted(d['intrinsics']))"
```
Expected: `['54138969', '55011271', '58860488', '60457274']`

- [ ] **Step 9: Write `scripts/fetch-data.sh`**

```bash
#!/usr/bin/env bash
# Downloads the challenge data. Not committed: the frames are Human3.6M imagery.
set -euo pipefail
mkdir -p data
curl -L -o data/Pose.zip https://ics.uci.edu/~wayne/research/students/Pose.zip
unzip -o -q data/Pose.zip -d data
test -f data/Pose/poses.txt && echo "data ready: data/Pose"
```

Run: `chmod +x scripts/fetch-data.sh && ./scripts/fetch-data.sh`
Expected: `data ready: data/Pose`

- [ ] **Step 10: Write `README.md`**

````markdown
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
````

- [ ] **Step 11: Commit**

```bash
git add CMakeLists.txt cmake/ .gitignore README.md scripts/ src/ tests/ third_party/
git commit -m "chore: project scaffold, pinned dependencies, vendored H36M calibration"
```

---

### Task 2: `pose_io` — parse the challenge data

**Files:**
- Create: `src/pose_io.hpp`, `src/pose_io.cpp`, `tests/fixtures/mini_poses.txt`
- Test: `tests/test_pose_io.cpp`
- Modify: `CMakeLists.txt` (replace `src/placeholder.cpp`, add the test file)
- Delete: `src/placeholder.cpp`, `tests/test_smoke.cpp`

**Interfaces:**
- Consumes: nothing.
- Produces:
  - `pose::kJointCount == 14`
  - `struct pose::Frame { glm::dvec3 camera_position; std::array<glm::dvec3, 14> joints; }`
  - `std::vector<pose::Frame> pose::load_poses(const std::filesystem::path&)` — throws `std::runtime_error` on malformed input
  - `double pose::load_focal(const std::filesystem::path&)`
  - `std::vector<std::string> pose::load_joint_names(const std::filesystem::path&)`

- [ ] **Step 1: Write the fixture `tests/fixtures/mini_poses.txt`**

Two rows of 45 whitespace-separated values. Row 1 is camera at (1000, 0, 0) with all 14 joints at the origin except joint 0 at (1, 2, 3); row 2 is camera at (0, 2000, 0) with all joints at (5, 5, 5):

```
1000 0 0 1 2 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
0 2000 0 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5 5
```

- [ ] **Step 2: Write the failing test `tests/test_pose_io.cpp`**

```cpp
#include <doctest/doctest.h>
#include "pose_io.hpp"
#include <fstream>

TEST_CASE("load_poses reads camera position and joints") {
  const auto frames = pose::load_poses("tests/fixtures/mini_poses.txt");
  REQUIRE(frames.size() == 2);
  CHECK(frames[0].camera_position.x == doctest::Approx(1000.0));
  CHECK(frames[0].joints[0].z == doctest::Approx(3.0));
  CHECK(frames[0].joints[13].x == doctest::Approx(0.0));
  CHECK(frames[1].camera_position.y == doctest::Approx(2000.0));
  CHECK(frames[1].joints[7].y == doctest::Approx(5.0));
}

TEST_CASE("load_poses rejects a short row") {
  std::ofstream("build/bad_poses.txt") << "1 2 3 4 5\n";
  CHECK_THROWS_AS(pose::load_poses("build/bad_poses.txt"), std::runtime_error);
}

TEST_CASE("load_poses rejects a non-finite value") {
  std::ofstream out("build/nan_poses.txt");
  out << "nan 0 0";
  for (int i = 0; i < 42; ++i) out << " 0";
  out << "\n";
  out.close();
  CHECK_THROWS_AS(pose::load_poses("build/nan_poses.txt"), std::runtime_error);
}

TEST_CASE("load_focal and load_joint_names") {
  std::ofstream("build/focal.txt") << "1148.6\n";
  CHECK(pose::load_focal("build/focal.txt") == doctest::Approx(1148.6));
  std::ofstream("build/joints.txt") << "0   'Hip'\n1   'RHip'\n";
  const auto names = pose::load_joint_names("build/joints.txt");
  REQUIRE(names.size() == 2);
  CHECK(names[0] == "Hip");
  CHECK(names[1] == "RHip");
}
```

- [ ] **Step 3: Wire the new files into CMake and run the test to see it fail**

In `CMakeLists.txt`, replace `target_sources(pose_core PRIVATE src/placeholder.cpp)` with `target_sources(pose_core PRIVATE src/pose_io.cpp)`, and replace the `add_executable(pose_tests ...)` line with:

```cmake
add_executable(pose_tests tests/main_tests.cpp tests/test_pose_io.cpp)
```

Create `tests/main_tests.cpp` containing exactly:

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
```

Then delete `src/placeholder.cpp` and `tests/test_smoke.cpp`.

Run: `cmake -S . -B build && cmake --build build -j 2>&1 | tail -5`
Expected: FAIL — `pose_io.hpp: No such file or directory`.

- [ ] **Step 4: Write `src/pose_io.hpp`**

```cpp
#pragma once
#include <array>
#include <filesystem>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

namespace pose {

inline constexpr int kJointCount = 14;
inline constexpr int kPoseColumns = 3 + 3 * kJointCount;  // 45

struct Frame {
  glm::dvec3 camera_position{};
  std::array<glm::dvec3, kJointCount> joints{};
};

std::vector<Frame> load_poses(const std::filesystem::path& path);
double load_focal(const std::filesystem::path& path);
std::vector<std::string> load_joint_names(const std::filesystem::path& path);

}  // namespace pose
```

- [ ] **Step 5: Write `src/pose_io.cpp`**

```cpp
#include "pose_io.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace pose {
namespace {

[[noreturn]] void fail(const std::filesystem::path& p, std::size_t line, const std::string& why) {
  throw std::runtime_error(p.string() + ":" + std::to_string(line) + ": " + why);
}

}  // namespace

std::vector<Frame> load_poses(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open " + path.string());

  std::vector<Frame> frames;
  std::string line;
  std::size_t line_no = 0;
  while (std::getline(in, line)) {
    ++line_no;
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

    std::istringstream ss(line);
    std::vector<double> values;
    double v = 0.0;
    while (ss >> v) values.push_back(v);
    if (!ss.eof()) fail(path, line_no, "non-numeric token in row");
    if (values.size() != static_cast<std::size_t>(kPoseColumns))
      fail(path, line_no, "expected " + std::to_string(kPoseColumns) + " columns, got " +
                              std::to_string(values.size()));
    for (double x : values)
      if (!std::isfinite(x)) fail(path, line_no, "non-finite value");

    Frame f;
    f.camera_position = {values[0], values[1], values[2]};
    for (int j = 0; j < kJointCount; ++j)
      f.joints[j] = {values[3 + 3 * j], values[4 + 3 * j], values[5 + 3 * j]};
    frames.push_back(f);
  }
  if (frames.empty()) throw std::runtime_error("no pose rows in " + path.string());
  return frames;
}

double load_focal(const std::filesystem::path& path) {
  std::ifstream in(path);
  double f = 0.0;
  if (!in || !(in >> f) || !std::isfinite(f) || f <= 0.0)
    throw std::runtime_error("bad focal length in " + path.string());
  return f;
}

std::vector<std::string> load_joint_names(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open " + path.string());
  std::vector<std::string> names;
  std::string line;
  while (std::getline(in, line)) {
    const auto a = line.find('\'');
    const auto b = line.rfind('\'');
    if (a == std::string::npos || b <= a) continue;
    names.push_back(line.substr(a + 1, b - a - 1));
  }
  if (names.empty()) throw std::runtime_error("no joint names in " + path.string());
  return names;
}

}  // namespace pose
```

- [ ] **Step 6: Build and run the tests**

Run: `cmake --build build -j && ./build/pose_tests`
Expected: PASS, 4 test cases.

- [ ] **Step 7: Verify against the real data**

Run:
```bash
./build/pose_tests && python3 -c "
rows=[l.split() for l in open('data/Pose/poses.txt') if l.strip()]
print('rows',len(rows),'cols',{len(r) for r in rows})"
```
Expected: `rows 20 cols {45}` — confirming the parser's constants match reality.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat: parse poses, focal length and joint names"
```

---

### Task 3: `skeleton` — bone topology and limb colouring

**Files:**
- Create: `src/skeleton.hpp`, `src/skeleton.cpp`
- Test: `tests/test_skeleton.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `pose::kJointCount`.
- Produces:
  - `enum class pose::Side { Left, Right, Torso }`
  - `struct pose::Bone { int a; int b; Side side; }`
  - `const std::array<pose::Bone, 13>& pose::bones()`

- [ ] **Step 1: Write the failing test `tests/test_skeleton.cpp`**

```cpp
#include <doctest/doctest.h>
#include "skeleton.hpp"
#include "pose_io.hpp"
#include <set>

TEST_CASE("bones reference valid joints and form a tree") {
  const auto& bs = pose::bones();
  CHECK(bs.size() == 13);  // a 14-node tree has 13 edges
  std::set<int> touched;
  for (const auto& b : bs) {
    CHECK(b.a >= 0);
    CHECK(b.a < pose::kJointCount);
    CHECK(b.b >= 0);
    CHECK(b.b < pose::kJointCount);
    CHECK(b.a != b.b);
    touched.insert(b.a);
    touched.insert(b.b);
  }
  CHECK(touched.size() == static_cast<std::size_t>(pose::kJointCount));
}

TEST_CASE("left and right limbs are balanced") {
  int left = 0, right = 0;
  for (const auto& b : pose::bones()) {
    if (b.side == pose::Side::Left) ++left;
    if (b.side == pose::Side::Right) ++right;
  }
  CHECK(left == right);
  CHECK(left == 6);
}
```

- [ ] **Step 2: Add the test to CMake and run it to see it fail**

In `CMakeLists.txt`, append `tests/test_skeleton.cpp` to the `add_executable(pose_tests ...)` list and `src/skeleton.cpp` to `target_sources(pose_core ...)`.

Run: `cmake -S . -B build && cmake --build build -j 2>&1 | tail -3`
Expected: FAIL — `skeleton.hpp: No such file or directory`.

- [ ] **Step 3: Write `src/skeleton.hpp`**

```cpp
#pragma once
#include <array>

namespace pose {

enum class Side { Left, Right, Torso };

struct Bone {
  int a;
  int b;
  Side side;
};

// Joint indices, from joint-names.txt:
// 0 Hip, 1 RHip, 2 RKnee, 3 RAnkle, 4 LHip, 5 LKnee, 6 LAnkle, 7 Neck,
// 8 LUpperArm, 9 LElbow, 10 LWrist, 11 RUpperArm, 12 RElbow, 13 RWrist
const std::array<Bone, 13>& bones();

}  // namespace pose
```

- [ ] **Step 4: Write `src/skeleton.cpp`**

```cpp
#include "skeleton.hpp"

namespace pose {

const std::array<Bone, 13>& bones() {
  static const std::array<Bone, 13> kBones{{
      {0, 1, Side::Right},   {1, 2, Side::Right},   {2, 3, Side::Right},
      {0, 4, Side::Left},    {4, 5, Side::Left},    {5, 6, Side::Left},
      {0, 7, Side::Torso},
      {7, 8, Side::Left},    {8, 9, Side::Left},    {9, 10, Side::Left},
      {7, 11, Side::Right},  {11, 12, Side::Right}, {12, 13, Side::Right},
  }};
  return kBones;
}

}  // namespace pose
```

- [ ] **Step 5: Build and run the tests**

Run: `cmake --build build -j && ./build/pose_tests`
Expected: PASS, 6 test cases.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: skeleton bone topology and limb sides"
```

---

### Task 4: `camera` part 1 — look-at construction and pinhole projection

**Files:**
- Create: `src/camera.hpp`, `src/camera.cpp`
- Test: `tests/test_camera.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `pose::Frame`.
- Produces:
  - `struct pose::Intrinsics { double fx, fy, cx, cy; std::array<double,5> distortion; bool has_distortion; }`
  - `struct pose::Extrinsics { glm::dmat3 rotation; glm::dvec3 translation; }` — camera-space point is `rotation * world + translation`
  - `glm::dmat3 pose::look_at_rotation(const glm::dvec3& eye, const glm::dvec3& target, const glm::dvec3& world_up)` — rows are the camera basis (right, down, forward)
  - `pose::Extrinsics pose::look_at_extrinsics(const glm::dvec3& eye, const glm::dvec3& target, const glm::dvec3& world_up)`
  - `glm::dvec3 pose::centroid(const pose::Frame&)`
  - `glm::dvec2 pose::project(const glm::dvec3& world, const Extrinsics&, const Intrinsics&, bool apply_distortion)` — throws `std::runtime_error` when the point is at or behind the image plane
  - `pose::Intrinsics pose::challenge_intrinsics(double focal)` — `fx = fy = focal`, principal point (500, 500), no distortion

Convention note for the implementer: the camera looks down `+Z` in camera space; image `+x` is right, image `+y` is **down**. That is why the basis row for the vertical axis is `-up`.

- [ ] **Step 1: Write the failing test `tests/test_camera.cpp`**

```cpp
#include <doctest/doctest.h>
#include "camera.hpp"
#include <glm/gtc/matrix_access.hpp>

TEST_CASE("look_at basis is orthonormal and right-handed") {
  const auto R = pose::look_at_rotation({1761.0, -5078.0, 1606.0}, {0.0, 0.0, 150.0}, {0, 0, 1});
  for (int i = 0; i < 3; ++i) CHECK(glm::length(glm::row(R, i)) == doctest::Approx(1.0));
  for (int i = 0; i < 3; ++i)
    for (int j = i + 1; j < 3; ++j)
      CHECK(glm::dot(glm::row(R, i), glm::row(R, j)) == doctest::Approx(0.0).epsilon(1e-12));
  CHECK(glm::determinant(R) == doctest::Approx(1.0));
}

TEST_CASE("the look-at target lands exactly on the principal point") {
  const glm::dvec3 eye{1761.0, -5078.0, 1606.0};
  const glm::dvec3 target{12.0, -34.0, 156.0};
  const auto ext = pose::look_at_extrinsics(eye, target, {0, 0, 1});
  const auto k = pose::challenge_intrinsics(1148.6);
  const auto uv = pose::project(target, ext, k, false);
  CHECK(uv.x == doctest::Approx(500.0));
  CHECK(uv.y == doctest::Approx(500.0));
}

TEST_CASE("known synthetic camera maps a known point to a known pixel") {
  // Camera at the origin looking down +Z, identity rotation.
  pose::Extrinsics ext{glm::dmat3(1.0), glm::dvec3(0.0)};
  pose::Intrinsics k{1000.0, 1000.0, 500.0, 500.0, {}, false};
  // A point 2 m away, 1 m right and 0.5 m down: u = 1000*1000/2000 + 500 = 1000.
  const auto uv = pose::project({1000.0, 500.0, 2000.0}, ext, k, false);
  CHECK(uv.x == doctest::Approx(1000.0));
  CHECK(uv.y == doctest::Approx(750.0));
}

TEST_CASE("projection is invariant to uniform scaling of the scene") {
  pose::Extrinsics ext{glm::dmat3(1.0), glm::dvec3(0.0)};
  pose::Intrinsics k{1000.0, 1000.0, 500.0, 500.0, {}, false};
  const auto a = pose::project({100.0, 50.0, 200.0}, ext, k, false);
  const auto b = pose::project({1000.0, 500.0, 2000.0}, ext, k, false);
  CHECK(a.x == doctest::Approx(b.x));
  CHECK(a.y == doctest::Approx(b.y));
}

TEST_CASE("a point behind the camera is rejected, not silently clipped") {
  pose::Extrinsics ext{glm::dmat3(1.0), glm::dvec3(0.0)};
  pose::Intrinsics k{1000.0, 1000.0, 500.0, 500.0, {}, false};
  CHECK_THROWS_AS(pose::project({0.0, 0.0, -100.0}, ext, k, false), std::runtime_error);
}

TEST_CASE("centroid averages all 14 joints") {
  pose::Frame f;
  for (auto& j : f.joints) j = {3.0, 6.0, 9.0};
  const auto c = pose::centroid(f);
  CHECK(c.x == doctest::Approx(3.0));
  CHECK(c.z == doctest::Approx(9.0));
}
```

- [ ] **Step 2: Add to CMake and run to see it fail**

Append `tests/test_camera.cpp` to `pose_tests` and `src/camera.cpp` to `pose_core`.

Run: `cmake -S . -B build && cmake --build build -j 2>&1 | tail -3`
Expected: FAIL — `camera.hpp: No such file or directory`.

- [ ] **Step 3: Write `src/camera.hpp`**

```cpp
#pragma once
#include <array>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "pose_io.hpp"

namespace pose {

struct Intrinsics {
  double fx{};
  double fy{};
  double cx{};
  double cy{};
  std::array<double, 5> distortion{};  // OpenCV order: k1, k2, p1, p2, k3
  bool has_distortion{false};
};

// Camera-space point = rotation * world + translation.
struct Extrinsics {
  glm::dmat3 rotation{1.0};
  glm::dvec3 translation{0.0};
};

glm::dmat3 look_at_rotation(const glm::dvec3& eye, const glm::dvec3& target,
                            const glm::dvec3& world_up);
Extrinsics look_at_extrinsics(const glm::dvec3& eye, const glm::dvec3& target,
                              const glm::dvec3& world_up);
glm::dvec3 centroid(const Frame& frame);
glm::dvec2 project(const glm::dvec3& world, const Extrinsics& ext, const Intrinsics& k,
                   bool apply_distortion);
Intrinsics challenge_intrinsics(double focal);

}  // namespace pose
```

- [ ] **Step 4: Write `src/camera.cpp`**

```cpp
#include "camera.hpp"

#include <glm/geometric.hpp>
#include <stdexcept>

namespace pose {

glm::dmat3 look_at_rotation(const glm::dvec3& eye, const glm::dvec3& target,
                            const glm::dvec3& world_up) {
  const glm::dvec3 forward = glm::normalize(target - eye);
  glm::dvec3 right = glm::cross(forward, world_up);
  const double len = glm::length(right);
  if (len < 1e-9)
    throw std::runtime_error("look_at: view direction is parallel to the world-up axis");
  right /= len;
  const glm::dvec3 up = glm::cross(right, forward);

  // Rows are the camera basis; image y points down, hence -up.
  glm::dmat3 R(1.0);
  for (int c = 0; c < 3; ++c) {
    R[c][0] = right[c];
    R[c][1] = -up[c];
    R[c][2] = forward[c];
  }
  return R;
}

Extrinsics look_at_extrinsics(const glm::dvec3& eye, const glm::dvec3& target,
                              const glm::dvec3& world_up) {
  Extrinsics e;
  e.rotation = look_at_rotation(eye, target, world_up);
  e.translation = -(e.rotation * eye);
  return e;
}

glm::dvec3 centroid(const Frame& frame) {
  glm::dvec3 sum(0.0);
  for (const auto& j : frame.joints) sum += j;
  return sum / static_cast<double>(kJointCount);
}

glm::dvec2 project(const glm::dvec3& world, const Extrinsics& ext, const Intrinsics& k,
                   bool apply_distortion) {
  const glm::dvec3 c = ext.rotation * world + ext.translation;
  if (c.z <= 1e-6) throw std::runtime_error("point is at or behind the image plane");

  double x = c.x / c.z;
  double y = c.y / c.z;

  if (apply_distortion && k.has_distortion) {
    const double k1 = k.distortion[0], k2 = k.distortion[1];
    const double p1 = k.distortion[2], p2 = k.distortion[3], k3 = k.distortion[4];
    const double r2 = x * x + y * y;
    const double radial = 1.0 + r2 * (k1 + r2 * (k2 + r2 * k3));
    const double xd = x * radial + 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
    const double yd = y * radial + p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
    x = xd;
    y = yd;
  }
  return {k.fx * x + k.cx, k.fy * y + k.cy};
}

Intrinsics challenge_intrinsics(double focal) {
  Intrinsics k;
  k.fx = focal;
  k.fy = focal;
  k.cx = 500.0;
  k.cy = 500.0;
  k.has_distortion = false;
  return k;
}

}  // namespace pose
```

- [ ] **Step 5: Build and run the tests**

Run: `cmake --build build -j && ./build/pose_tests`
Expected: PASS, 12 test cases.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: look-at construction and pinhole projection"
```

---

### Task 5: `camera` part 2 — published calibration and camera identification

**Files:**
- Modify: `src/camera.hpp`, `src/camera.cpp`, `tests/test_camera.cpp`, `CMakeLists.txt`

**Interfaces:**
- Consumes: everything from Task 4.
- Produces:
  - `struct pose::CalibratedCamera { std::string id; Intrinsics intrinsics; Extrinsics extrinsics; glm::dvec3 center; }`
  - `std::vector<pose::CalibratedCamera> pose::load_calibration(const std::filesystem::path& json, const std::string& subject)` — default subject `"S1"`
  - `const pose::CalibratedCamera* pose::identify(const glm::dvec3& camera_position, const std::vector<CalibratedCamera>&, double tolerance_mm)` — nullptr when nothing is within tolerance
  - `glm::dvec3 pose::camera_center(const Extrinsics&)` — `-Rᵀt`

- [ ] **Step 1: Write the failing tests (append to `tests/test_camera.cpp`)**

```cpp
#include <filesystem>

TEST_CASE("calibration loads four S1 cameras with plausible focals") {
  const auto cams = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");
  REQUIRE(cams.size() == 4);
  for (const auto& c : cams) {
    CHECK(c.intrinsics.fx > 1100.0);
    CHECK(c.intrinsics.fx < 1200.0);
    CHECK(c.intrinsics.has_distortion);
  }
}

TEST_CASE("camera centers match the positions in the challenge file exactly") {
  const auto cams = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");
  // Row 0 of the real poses.txt.
  const glm::dvec3 row0{1761.27853428116, -5078.00659454077, 1606.2649598335};
  const auto* hit = pose::identify(row0, cams, 1.0);
  REQUIRE(hit != nullptr);
  CHECK(hit->id == "55011271");
  CHECK(glm::length(hit->center - row0) < 0.01);
}

TEST_CASE("identify returns nullptr past tolerance") {
  const auto cams = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");
  CHECK(pose::identify({0.0, 0.0, 0.0}, cams, 100.0) == nullptr);
}

TEST_CASE("ground-truth projection lands where the subject sits in frame 00") {
  const auto cams = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");
  const auto frames = pose::load_poses("data/Pose/poses.txt");
  REQUIRE(frames.size() == 20);
  const auto* cam = pose::identify(frames[0].camera_position, cams, 1.0);
  REQUIRE(cam != nullptr);
  glm::dvec2 sum(0.0);
  for (const auto& j : frames[0].joints)
    sum += pose::project(j, cam->extrinsics, cam->intrinsics, false);
  const glm::dvec2 c = sum / 14.0;
  CHECK(c.x == doctest::Approx(559.3).epsilon(0.01));
  CHECK(c.y == doctest::Approx(547.1).epsilon(0.01));
}
```

Note for the implementer: the last test needs `./scripts/fetch-data.sh` to have been run, and the test binary must be executed from the repository root.

- [ ] **Step 2: Run to see it fail**

Run: `cmake --build build -j 2>&1 | tail -3`
Expected: FAIL — `load_calibration` is not a member of `pose`.

- [ ] **Step 3: Extend `src/camera.hpp`**

Add near the top: `#include <filesystem>`, `#include <string>`, `#include <vector>`. Add before the closing namespace brace:

```cpp
struct CalibratedCamera {
  std::string id;
  Intrinsics intrinsics;
  Extrinsics extrinsics;
  glm::dvec3 center{};
};

glm::dvec3 camera_center(const Extrinsics& ext);
std::vector<CalibratedCamera> load_calibration(const std::filesystem::path& json_path,
                                               const std::string& subject = "S1");
const CalibratedCamera* identify(const glm::dvec3& camera_position,
                                 const std::vector<CalibratedCamera>& cameras,
                                 double tolerance_mm);
```

- [ ] **Step 4: Extend `src/camera.cpp`**

Add `#include <fstream>`, `#include <nlohmann/json.hpp>` at the top, and these definitions before the closing namespace brace:

```cpp
glm::dvec3 camera_center(const Extrinsics& ext) {
  return -(glm::transpose(ext.rotation) * ext.translation);
}

std::vector<CalibratedCamera> load_calibration(const std::filesystem::path& json_path,
                                               const std::string& subject) {
  std::ifstream in(json_path);
  if (!in) throw std::runtime_error("cannot open " + json_path.string());
  const nlohmann::json root = nlohmann::json::parse(in);
  if (!root.contains("extrinsics") || !root["extrinsics"].contains(subject))
    throw std::runtime_error("no extrinsics for subject " + subject);

  std::vector<CalibratedCamera> cameras;
  for (const auto& [id, ext_json] : root["extrinsics"][subject].items()) {
    CalibratedCamera cam;
    cam.id = id;

    const auto& K = root["intrinsics"][id]["calibration_matrix"];
    cam.intrinsics.fx = K[0][0].get<double>();
    cam.intrinsics.fy = K[1][1].get<double>();
    cam.intrinsics.cx = K[0][2].get<double>();
    cam.intrinsics.cy = K[1][2].get<double>();
    const auto& d = root["intrinsics"][id]["distortion"];
    for (int i = 0; i < 5; ++i) cam.intrinsics.distortion[i] = d[i].get<double>();
    cam.intrinsics.has_distortion = true;

    // R is row-major 3x3; glm::dmat3 is column-major, so index as R[col][row].
    const auto& R = ext_json["R"];
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) cam.extrinsics.rotation[c][r] = R[r][c].get<double>();

    const auto& t = ext_json["t"];
    for (int r = 0; r < 3; ++r)
      cam.extrinsics.translation[r] = t[r].is_array() ? t[r][0].get<double>() : t[r].get<double>();

    cam.center = camera_center(cam.extrinsics);
    cameras.push_back(std::move(cam));
  }
  if (cameras.empty()) throw std::runtime_error("no cameras for subject " + subject);
  return cameras;
}

const CalibratedCamera* identify(const glm::dvec3& camera_position,
                                 const std::vector<CalibratedCamera>& cameras,
                                 double tolerance_mm) {
  const CalibratedCamera* best = nullptr;
  double best_distance = tolerance_mm;
  for (const auto& cam : cameras) {
    const double d = glm::length(cam.center - camera_position);
    if (d <= best_distance) {
      best_distance = d;
      best = &cam;
    }
  }
  return best;
}
```

- [ ] **Step 5: Build and run the tests from the repository root**

Run: `cmake --build build -j && ./build/pose_tests`
Expected: PASS, 16 test cases. The last one is the load-bearing one: it proves the calibration, the rotation convention, and the projection all agree with reality.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: load published H36M calibration and identify cameras by position"
```

---

### Task 6: `analysis` — error metrics and CSV output

**Files:**
- Create: `src/analysis.hpp`, `src/analysis.cpp`
- Test: `tests/test_analysis.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `pose::Frame`, `pose::Extrinsics`, `pose::Intrinsics`, `pose::bones()`.
- Produces:
  - `struct pose::FrameError { double mean_px; double max_px; int worst_joint; }`
  - `pose::FrameError pose::joint_error(const std::vector<glm::dvec2>& a, const std::vector<glm::dvec2>& b)`
  - `double pose::angular_error_degrees(const glm::dmat3& a, const glm::dmat3& b)`
  - `std::array<double, 13> pose::limb_lengths(const pose::Frame&)`
  - `void pose::write_csv(const std::filesystem::path&, const std::vector<std::string>& header, const std::vector<std::vector<std::string>>& rows)`
  - `std::string pose::fixed(double value, int decimals)` — deterministic number formatting used by every CSV writer

- [ ] **Step 1: Write the failing test `tests/test_analysis.cpp`**

```cpp
#include <doctest/doctest.h>
#include "analysis.hpp"
#include <fstream>
#include <sstream>

TEST_CASE("joint_error reports mean, max and the worst joint") {
  const std::vector<glm::dvec2> a{{0, 0}, {0, 0}, {0, 0}};
  const std::vector<glm::dvec2> b{{3, 4}, {0, 0}, {0, 1}};  // distances 5, 0, 1
  const auto e = pose::joint_error(a, b);
  CHECK(e.mean_px == doctest::Approx(2.0));
  CHECK(e.max_px == doctest::Approx(5.0));
  CHECK(e.worst_joint == 0);
}

TEST_CASE("angular_error is zero for identical rotations") {
  const glm::dmat3 I(1.0);
  CHECK(pose::angular_error_degrees(I, I) == doctest::Approx(0.0));
}

TEST_CASE("angular_error measures a known 90 degree rotation") {
  glm::dmat3 R(0.0);
  R[0][1] = 1.0;   // x -> y
  R[1][0] = -1.0;  // y -> -x
  R[2][2] = 1.0;
  CHECK(pose::angular_error_degrees(glm::dmat3(1.0), R) == doctest::Approx(90.0).epsilon(1e-9));
}

TEST_CASE("limb_lengths are positive and stable under translation") {
  pose::Frame f;
  for (int i = 0; i < pose::kJointCount; ++i) f.joints[i] = {i * 10.0, 0.0, 0.0};
  const auto a = pose::limb_lengths(f);
  for (auto& j : f.joints) j += glm::dvec3{1000.0, -500.0, 25.0};
  const auto b = pose::limb_lengths(f);
  for (std::size_t i = 0; i < a.size(); ++i) {
    CHECK(a[i] > 0.0);
    CHECK(a[i] == doctest::Approx(b[i]));
  }
}

TEST_CASE("fixed formats deterministically") {
  CHECK(pose::fixed(1.0 / 3.0, 3) == "0.333");
  CHECK(pose::fixed(-0.0004, 3) == "-0.000");
}

TEST_CASE("write_csv emits a header and quoted-free rows") {
  pose::write_csv("build/test.csv", {"a", "b"}, {{"1", "2"}, {"3", "4"}});
  std::ifstream in("build/test.csv");
  std::stringstream ss;
  ss << in.rdbuf();
  CHECK(ss.str() == "a,b\n1,2\n3,4\n");
}
```

- [ ] **Step 2: Add to CMake and run to see it fail**

Append `tests/test_analysis.cpp` to `pose_tests` and `src/analysis.cpp` to `pose_core`.

Run: `cmake -S . -B build && cmake --build build -j 2>&1 | tail -3`
Expected: FAIL — `analysis.hpp: No such file or directory`.

- [ ] **Step 3: Write `src/analysis.hpp`**

```cpp
#pragma once
#include <array>
#include <filesystem>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <string>
#include <vector>

#include "pose_io.hpp"

namespace pose {

struct FrameError {
  double mean_px{};
  double max_px{};
  int worst_joint{-1};
};

FrameError joint_error(const std::vector<glm::dvec2>& a, const std::vector<glm::dvec2>& b);
double angular_error_degrees(const glm::dmat3& a, const glm::dmat3& b);
std::array<double, 13> limb_lengths(const Frame& frame);
std::string fixed(double value, int decimals);
void write_csv(const std::filesystem::path& path, const std::vector<std::string>& header,
               const std::vector<std::vector<std::string>>& rows);

}  // namespace pose
```

- [ ] **Step 4: Write `src/analysis.cpp`**

```cpp
#include "analysis.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <glm/geometric.hpp>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "skeleton.hpp"

namespace pose {

FrameError joint_error(const std::vector<glm::dvec2>& a, const std::vector<glm::dvec2>& b) {
  if (a.size() != b.size() || a.empty()) throw std::runtime_error("joint_error: size mismatch");
  FrameError e;
  double sum = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const double d = glm::length(a[i] - b[i]);
    sum += d;
    if (d > e.max_px) {
      e.max_px = d;
      e.worst_joint = static_cast<int>(i);
    }
  }
  e.mean_px = sum / static_cast<double>(a.size());
  return e;
}

double angular_error_degrees(const glm::dmat3& a, const glm::dmat3& b) {
  const glm::dmat3 d = glm::transpose(a) * b;
  const double trace = d[0][0] + d[1][1] + d[2][2];
  const double cos_theta = std::clamp((trace - 1.0) / 2.0, -1.0, 1.0);
  return std::acos(cos_theta) * 180.0 / M_PI;
}

std::array<double, 13> limb_lengths(const Frame& frame) {
  std::array<double, 13> lengths{};
  const auto& bs = bones();
  for (std::size_t i = 0; i < bs.size(); ++i)
    lengths[i] = glm::length(frame.joints[bs[i].b] - frame.joints[bs[i].a]);
  return lengths;
}

std::string fixed(double value, int decimals) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::fixed << std::setprecision(decimals) << value;
  return out.str();
}

void write_csv(const std::filesystem::path& path, const std::vector<std::string>& header,
               const std::vector<std::vector<std::string>>& rows) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) throw std::runtime_error("cannot write " + path.string());
  for (std::size_t i = 0; i < header.size(); ++i) out << (i ? "," : "") << header[i];
  out << "\n";
  for (const auto& row : rows) {
    if (row.size() != header.size()) throw std::runtime_error("write_csv: column count mismatch");
    for (std::size_t i = 0; i < row.size(); ++i) out << (i ? "," : "") << row[i];
    out << "\n";
  }
}

}  // namespace pose
```

- [ ] **Step 5: Build and run the tests**

Run: `cmake --build build -j && ./build/pose_tests`
Expected: PASS, 22 test cases.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: reprojection error, angular error, limb lengths and CSV output"
```

---

### Task 7: `draw` + `render` — the 20 white-background renders

**Files:**
- Create: `src/draw.hpp`, `src/draw.cpp`, `src/render.hpp`, `src/render.cpp`, `src/main_project.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `pose::bones()`, `pose::project`, `pose::look_at_extrinsics`, `pose::load_calibration`, `pose::identify`.
- Produces:
  - `void pose::draw_skeleton_2d(const std::vector<glm::dvec2>& uv, float thickness, float alpha)` — must be called between raylib `BeginDrawing`/`BeginTextureMode` and its `End...`
  - `enum class pose::Mode { LookAt, Gt }`
  - `std::vector<glm::dvec2> pose::project_frame(const pose::Frame&, Mode, const std::vector<pose::CalibratedCamera>&, double focal)`
  - `void pose::render_white(const std::vector<glm::dvec2>& uv, const std::filesystem::path& out_png)`
  - Binary `pose-project` with `--data <dir> --out <dir> --mode lookat|gt|both`

- [ ] **Step 1: Write `src/draw.hpp`**

```cpp
#pragma once
#include <glm/vec2.hpp>
#include <vector>

namespace pose {

// Colours follow the challenge page's sample figure: right limbs red,
// left limbs blue, torso black. Call inside an active raylib draw scope.
void draw_skeleton_2d(const std::vector<glm::dvec2>& uv, float thickness, float alpha);

}  // namespace pose
```

- [ ] **Step 2: Write `src/draw.cpp`**

```cpp
#include "draw.hpp"

#include <raylib.h>

#include "skeleton.hpp"

namespace pose {
namespace {

Color side_color(Side side, float alpha) {
  const unsigned char a = static_cast<unsigned char>(alpha * 255.0f);
  switch (side) {
    case Side::Left:  return Color{40, 90, 220, a};
    case Side::Right: return Color{220, 50, 50, a};
    default:          return Color{20, 20, 20, a};
  }
}

}  // namespace

void draw_skeleton_2d(const std::vector<glm::dvec2>& uv, float thickness, float alpha) {
  for (const auto& b : bones()) {
    const Vector2 p{static_cast<float>(uv[b.a].x), static_cast<float>(uv[b.a].y)};
    const Vector2 q{static_cast<float>(uv[b.b].x), static_cast<float>(uv[b.b].y)};
    DrawLineEx(p, q, thickness, side_color(b.side, alpha));
  }
  for (const auto& p : uv)
    DrawCircle(static_cast<int>(p.x), static_cast<int>(p.y), thickness * 0.9f,
               Color{20, 20, 20, static_cast<unsigned char>(alpha * 255.0f)});
}

}  // namespace pose
```

- [ ] **Step 3: Write `src/render.hpp`**

```cpp
#pragma once
#include <filesystem>
#include <glm/vec2.hpp>
#include <vector>

#include "camera.hpp"
#include "pose_io.hpp"

namespace pose {

inline constexpr int kImageSize = 1000;

enum class Mode { LookAt, Gt };

// Projects all 14 joints of a frame. LookAt uses only challenge-supplied data;
// Gt uses the published calibration and throws if the camera cannot be identified.
std::vector<glm::dvec2> project_frame(const Frame& frame, Mode mode,
                                      const std::vector<CalibratedCamera>& cameras, double focal);

// Offscreen rendering. Requires a GL context; run under xvfb-run if headless.
void begin_offscreen();
void end_offscreen();
void render_white(const std::vector<glm::dvec2>& uv, const std::filesystem::path& out_png);

}  // namespace pose
```

- [ ] **Step 4: Write `src/render.cpp`**

```cpp
#include "render.hpp"

#include <raylib.h>

#include <stdexcept>

#include "draw.hpp"

namespace pose {

std::vector<glm::dvec2> project_frame(const Frame& frame, Mode mode,
                                      const std::vector<CalibratedCamera>& cameras, double focal) {
  std::vector<glm::dvec2> uv;
  uv.reserve(kJointCount);

  if (mode == Mode::LookAt) {
    const auto ext = look_at_extrinsics(frame.camera_position, centroid(frame), {0.0, 0.0, 1.0});
    const auto k = challenge_intrinsics(focal);
    for (const auto& j : frame.joints) uv.push_back(project(j, ext, k, false));
    return uv;
  }

  const auto* cam = identify(frame.camera_position, cameras, 1.0);
  if (cam == nullptr)
    throw std::runtime_error("no published camera matches this position; use --mode lookat");
  for (const auto& j : frame.joints) uv.push_back(project(j, cam->extrinsics, cam->intrinsics, false));
  return uv;
}

void begin_offscreen() {
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_MSAA_4X_HINT);
  InitWindow(kImageSize, kImageSize, "pose-project");
  if (!IsWindowReady())
    throw std::runtime_error("no GL context: re-run under 'xvfb-run -a ./build/pose-project ...'");
}

void end_offscreen() { CloseWindow(); }

void render_white(const std::vector<glm::dvec2>& uv, const std::filesystem::path& out_png) {
  std::filesystem::create_directories(out_png.parent_path());
  RenderTexture2D target = LoadRenderTexture(kImageSize, kImageSize);
  BeginTextureMode(target);
  ClearBackground(RAYWHITE);
  draw_skeleton_2d(uv, 4.0f, 1.0f);
  EndTextureMode();

  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);  // render textures are bottom-up
  ExportImage(img, out_png.string().c_str());
  UnloadImage(img);
  UnloadRenderTexture(target);
}

}  // namespace pose
```

- [ ] **Step 5: Write `src/main_project.cpp`**

```cpp
#include <cstdio>
#include <iostream>
#include <string>

#include "analysis.hpp"
#include "render.hpp"

namespace {

struct Options {
  std::filesystem::path data{"data/Pose"};
  std::filesystem::path out{"out"};
  std::string mode{"both"};
};

Options parse_args(int argc, char** argv) {
  Options o;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const bool has_next = i + 1 < argc;
    if (a == "--data" && has_next) o.data = argv[++i];
    else if (a == "--out" && has_next) o.out = argv[++i];
    else if (a == "--mode" && has_next) o.mode = argv[++i];
    else throw std::runtime_error("usage: pose-project --data <dir> --out <dir> --mode lookat|gt|both");
  }
  if (o.mode != "lookat" && o.mode != "gt" && o.mode != "both")
    throw std::runtime_error("--mode must be lookat, gt or both");
  return o;
}

std::string frame_name(std::size_t i) {
  char buf[8];
  std::snprintf(buf, sizeof(buf), "%02zu", i);
  return buf;
}

}  // namespace

int main(int argc, char** argv) try {
  const Options opt = parse_args(argc, argv);

  const auto frames = pose::load_poses(opt.data / "poses.txt");
  const double focal = pose::load_focal(opt.data / "focal.txt");
  const auto cameras = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");

  pose::begin_offscreen();
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const auto mode = (opt.mode == "gt") ? pose::Mode::Gt : pose::Mode::LookAt;
    const auto uv = pose::project_frame(frames[i], mode, cameras, focal);
    pose::render_white(uv, opt.out / "white" / (frame_name(i) + ".png"));
  }
  pose::end_offscreen();

  std::cout << "wrote " << frames.size() << " white renders to " << (opt.out / "white") << "\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << "\n";
  return 1;
}
```

- [ ] **Step 6: Add the binary to CMake**

Append to `CMakeLists.txt`:

```cmake
add_executable(pose-project src/main_project.cpp)
target_link_libraries(pose-project PRIVATE pose_core)
```

and append `src/draw.cpp` and `src/render.cpp` to `target_sources(pose_core ...)`.

- [ ] **Step 7: Build and run it**

Run:
```bash
cmake -S . -B build && cmake --build build -j
./build/pose-project --data data/Pose --out out --mode lookat || \
  xvfb-run -a ./build/pose-project --data data/Pose --out out --mode lookat
ls out/white | wc -l
```
Expected: `wrote 20 white renders...` and `20`.

- [ ] **Step 8: Eyeball one render**

Run: `ls -la out/white/00.png`
Expected: a non-trivial PNG (> 5 KB). Open it: a red/blue/black stick figure centred in a white 1000×1000 image. If the figure is upside down, `ImageFlipVertical` was applied twice — remove one.

- [ ] **Step 9: Commit**

```bash
git add -A
git commit -m "feat: batch renderer producing the 20 white-background projections"
```

---

### Task 8: Overlays on the real frames and the side-by-side panels

**Files:**
- Modify: `src/render.hpp`, `src/render.cpp`, `src/main_project.cpp`

**Interfaces:**
- Consumes: Task 7's `project_frame`, `draw_skeleton_2d`.
- Produces:
  - `void pose::render_overlay(const std::vector<glm::dvec2>& uv, const std::filesystem::path& frame_png, const std::filesystem::path& out_png)`
  - `void pose::render_panel(const std::filesystem::path& left_png, const std::filesystem::path& right_png, const std::filesystem::path& out_png)`

- [ ] **Step 1: Declare the two functions in `src/render.hpp`**

```cpp
// Draws the skeleton over the photograph. Skipped with a warning if the frame is missing.
void render_overlay(const std::vector<glm::dvec2>& uv, const std::filesystem::path& frame_png,
                    const std::filesystem::path& out_png);

// Composites two equally sized PNGs side by side, echoing the challenge page's sample figure.
void render_panel(const std::filesystem::path& left_png, const std::filesystem::path& right_png,
                  const std::filesystem::path& out_png);
```

- [ ] **Step 2: Implement them in `src/render.cpp`**

```cpp
void render_overlay(const std::vector<glm::dvec2>& uv, const std::filesystem::path& frame_png,
                    const std::filesystem::path& out_png) {
  if (!std::filesystem::exists(frame_png)) {
    TraceLog(LOG_WARNING, "missing frame %s; skipping overlay", frame_png.string().c_str());
    return;
  }
  std::filesystem::create_directories(out_png.parent_path());

  Texture2D photo = LoadTexture(frame_png.string().c_str());
  RenderTexture2D target = LoadRenderTexture(kImageSize, kImageSize);
  BeginTextureMode(target);
  ClearBackground(BLACK);
  DrawTexture(photo, 0, 0, WHITE);
  draw_skeleton_2d(uv, 4.0f, 1.0f);
  EndTextureMode();

  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);
  ExportImage(img, out_png.string().c_str());
  UnloadImage(img);
  UnloadRenderTexture(target);
  UnloadTexture(photo);
}

void render_panel(const std::filesystem::path& left_png, const std::filesystem::path& right_png,
                  const std::filesystem::path& out_png) {
  if (!std::filesystem::exists(left_png) || !std::filesystem::exists(right_png)) return;
  std::filesystem::create_directories(out_png.parent_path());

  Image left = LoadImage(left_png.string().c_str());
  Image right = LoadImage(right_png.string().c_str());
  Image panel = GenImageColor(left.width + right.width, left.height, RAYWHITE);
  ImageDraw(&panel, left, Rectangle{0, 0, (float)left.width, (float)left.height},
            Rectangle{0, 0, (float)left.width, (float)left.height}, WHITE);
  ImageDraw(&panel, right, Rectangle{0, 0, (float)right.width, (float)right.height},
            Rectangle{(float)left.width, 0, (float)right.width, (float)right.height}, WHITE);
  ExportImage(panel, out_png.string().c_str());
  UnloadImage(panel);
  UnloadImage(right);
  UnloadImage(left);
}
```

- [ ] **Step 3: Call them from `src/main_project.cpp`**

Replace the render loop body with:

```cpp
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const std::string name = frame_name(i);
    const auto mode = (opt.mode == "gt") ? pose::Mode::Gt : pose::Mode::LookAt;
    const auto uv = pose::project_frame(frames[i], mode, cameras, focal);

    const auto white_png = opt.out / "white" / (name + ".png");
    const auto overlay_png = opt.out / "overlay" / (name + ".png");
    pose::render_white(uv, white_png);

    // The overlay is only meaningful with the published calibration; a look-at
    // overlay would place the skeleton at the image centre regardless of the photo.
    const auto overlay_uv =
        (opt.mode == "lookat") ? uv : pose::project_frame(frames[i], pose::Mode::Gt, cameras, focal);
    pose::render_overlay(overlay_uv, opt.data / "frames" / (name + ".png"), overlay_png);
    pose::render_panel(overlay_png, white_png, opt.out / "panel" / (name + ".png"));
  }
```

- [ ] **Step 4: Build and run in `gt` mode**

Run:
```bash
cmake --build build -j
./build/pose-project --data data/Pose --out out --mode gt || \
  xvfb-run -a ./build/pose-project --data data/Pose --out out --mode gt
ls out/overlay | wc -l && ls out/panel | wc -l
```
Expected: `20` and `20`.

- [ ] **Step 5: Verify the overlay actually lands on the subject**

Open `out/overlay/00.png`. Expected: the stick figure sits on the seated woman — head, hips and limbs aligned within a few pixels. **If it does not, stop and debug: it means the rotation convention or the y-axis sign is wrong, and every later number is invalid.**

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: skeleton overlays on the photographs and side-by-side panels"
```

---

### Task 9: The consolidated coordinate table and the error analysis

**Files:**
- Modify: `src/main_project.cpp`
- Create: `src/tables.hpp`, `src/tables.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `pose::write_csv`, `pose::fixed`, `pose::joint_error`, `pose::angular_error_degrees`, `pose::project_frame`.
- Produces:
  - `void pose::write_coordinate_table(const std::filesystem::path& csv, const std::filesystem::path& tex, const std::vector<std::vector<glm::dvec2>>& lookat_uv, const std::vector<std::vector<glm::dvec2>>& gt_uv, const std::vector<std::string>& joint_names, const std::vector<std::string>& camera_ids)`
  - `void pose::write_error_tables(const std::filesystem::path& out_dir, const std::vector<std::vector<glm::dvec2>>& lookat_uv, const std::vector<std::vector<glm::dvec2>>& gt_uv, const std::vector<double>& angular_deg, const std::vector<std::string>& camera_ids)`

- [ ] **Step 1: Write `src/tables.hpp`**

```cpp
#pragma once
#include <filesystem>
#include <glm/vec2.hpp>
#include <string>
#include <vector>

namespace pose {

// The single table the challenge asks for: every 2D coordinate used, in both modes.
void write_coordinate_table(const std::filesystem::path& csv, const std::filesystem::path& tex,
                            const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                            const std::vector<std::vector<glm::dvec2>>& gt_uv,
                            const std::vector<std::string>& joint_names,
                            const std::vector<std::string>& camera_ids);

void write_error_tables(const std::filesystem::path& out_dir,
                        const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                        const std::vector<std::vector<glm::dvec2>>& gt_uv,
                        const std::vector<double>& angular_deg,
                        const std::vector<std::string>& camera_ids);

}  // namespace pose
```

- [ ] **Step 2: Write `src/tables.cpp`**

```cpp
#include "tables.hpp"

#include <fstream>

#include "analysis.hpp"
#include "pose_io.hpp"

namespace pose {

void write_coordinate_table(const std::filesystem::path& csv, const std::filesystem::path& tex,
                            const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                            const std::vector<std::vector<glm::dvec2>>& gt_uv,
                            const std::vector<std::string>& joint_names,
                            const std::vector<std::string>& camera_ids) {
  std::vector<std::vector<std::string>> rows;
  for (std::size_t f = 0; f < lookat_uv.size(); ++f) {
    for (int j = 0; j < kJointCount; ++j) {
      rows.push_back({std::to_string(f), camera_ids[f], std::to_string(j), joint_names[j],
                      fixed(lookat_uv[f][j].x, 2), fixed(lookat_uv[f][j].y, 2),
                      fixed(gt_uv[f][j].x, 2), fixed(gt_uv[f][j].y, 2)});
    }
  }
  write_csv(csv, {"frame", "camera", "joint", "name", "u_lookat", "v_lookat", "u_gt", "v_gt"}, rows);

  std::filesystem::create_directories(tex.parent_path());
  std::ofstream out(tex);
  out << "\\begin{longtable}{rlrlrrrr}\n\\toprule\n"
      << "Frame & Camera & \\# & Joint & $u_{\\text{look-at}}$ & $v_{\\text{look-at}}$ & "
         "$u_{\\text{gt}}$ & $v_{\\text{gt}}$ \\\\\n\\midrule\n\\endhead\n";
  for (const auto& r : rows) {
    for (std::size_t i = 0; i < r.size(); ++i) out << (i ? " & " : "") << r[i];
    out << " \\\\\n";
  }
  out << "\\bottomrule\n\\end{longtable}\n";
}

void write_error_tables(const std::filesystem::path& out_dir,
                        const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                        const std::vector<std::vector<glm::dvec2>>& gt_uv,
                        const std::vector<double>& angular_deg,
                        const std::vector<std::string>& camera_ids) {
  std::vector<std::vector<std::string>> rows;
  for (std::size_t f = 0; f < lookat_uv.size(); ++f) {
    const auto e = joint_error(lookat_uv[f], gt_uv[f]);
    rows.push_back({std::to_string(f), camera_ids[f], fixed(e.mean_px, 2), fixed(e.max_px, 2),
                    std::to_string(e.worst_joint), fixed(angular_deg[f], 4)});
  }
  write_csv(out_dir / "joint-error.csv",
            {"frame", "camera", "mean_px", "max_px", "worst_joint", "angular_deg"}, rows);
}

}  // namespace pose
```

- [ ] **Step 3: Rewrite the body of `main` in `src/main_project.cpp` to collect both modes**

```cpp
  pose::begin_offscreen();

  std::vector<std::vector<glm::dvec2>> lookat_uv, gt_uv;
  std::vector<std::string> camera_ids;
  std::vector<double> angular_deg;

  for (std::size_t i = 0; i < frames.size(); ++i) {
    const std::string name = frame_name(i);

    const auto la = pose::project_frame(frames[i], pose::Mode::LookAt, cameras, focal);
    const auto gt = pose::project_frame(frames[i], pose::Mode::Gt, cameras, focal);
    lookat_uv.push_back(la);
    gt_uv.push_back(gt);

    const auto* cam = pose::identify(frames[i].camera_position, cameras, 1.0);
    camera_ids.push_back(cam ? cam->id : "unknown");
    const auto la_ext =
        pose::look_at_extrinsics(frames[i].camera_position, pose::centroid(frames[i]), {0, 0, 1});
    angular_deg.push_back(
        cam ? pose::angular_error_degrees(la_ext.rotation, cam->extrinsics.rotation) : 0.0);

    const auto& primary = (opt.mode == "gt") ? gt : la;
    const auto white_png = opt.out / "white" / (name + ".png");
    const auto overlay_png = opt.out / "overlay" / (name + ".png");
    pose::render_white(primary, white_png);
    pose::render_overlay(gt, opt.data / "frames" / (name + ".png"), overlay_png);
    pose::render_panel(overlay_png, white_png, opt.out / "panel" / (name + ".png"));
  }
  pose::end_offscreen();

  const auto joint_names = pose::load_joint_names(opt.data / "joint-names.txt");
  pose::write_coordinate_table(opt.out / "coords" / "all-2d-coordinates.csv",
                               opt.out / "coords" / "all-2d-coordinates.tex", lookat_uv, gt_uv,
                               joint_names, camera_ids);
  pose::write_error_tables(opt.out / "analysis", lookat_uv, gt_uv, angular_deg, camera_ids);
  std::cout << "wrote renders, coordinate table and error analysis to " << opt.out << "\n";
```

Add `#include "tables.hpp"` at the top, and append `src/tables.cpp` to `target_sources(pose_core ...)`.

- [ ] **Step 4: Build, run, and check the table**

Run:
```bash
cmake -S . -B build && cmake --build build -j
./build/pose-project --data data/Pose --out out --mode both || \
  xvfb-run -a ./build/pose-project --data data/Pose --out out --mode both
wc -l out/coords/all-2d-coordinates.csv
head -3 out/coords/all-2d-coordinates.csv
column -s, -t out/analysis/joint-error.csv | head -6
```
Expected: `281` lines (1 header + 20 × 14), and a first data row for frame 0, camera `55011271`.

- [ ] **Step 5: Verify determinism**

Run:
```bash
cp out/coords/all-2d-coordinates.csv /tmp/first.csv
./build/pose-project --data data/Pose --out out --mode both >/dev/null || \
  xvfb-run -a ./build/pose-project --data data/Pose --out out --mode both >/dev/null
diff /tmp/first.csv out/coords/all-2d-coordinates.csv && echo "deterministic"
```
Expected: `deterministic`.

- [ ] **Step 6: Sanity-check the headline finding**

Run:
```bash
awk -F, 'NR>1 {s+=$3; if($4>m) m=$4} END {print "mean of means:", s/(NR-1), "worst:", m}' \
  out/analysis/joint-error.csv
```
Expected: a mean well above zero (tens of pixels) and a worst case in the low hundreds — the quantified look-at error the write-up is built on.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat: consolidated 2D coordinate table and reprojection error analysis"
```

---

### Task 10: `pose-explorer` — the plain debug tool

**Files:**
- Create: `src/main_explorer.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `pose::load_poses`, `pose::load_calibration`, `pose::project_frame`, `pose::draw_skeleton_2d`, `pose::joint_error`, `pose::bones()`.
- Produces: binary `pose-explorer`, `--data <dir>`.

Design constraint from the spec: **one full-window view at a time, plus a raygui panel of buttons, text and sliders. No split screen.**

- [ ] **Step 1: Write `src/main_explorer.cpp`**

```cpp
#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <cstdio>
#include <iostream>
#include <string>

#include "analysis.hpp"
#include "draw.hpp"
#include "render.hpp"

namespace {

constexpr int kWindow = 1000;

std::string frame_name(int i) {
  char buf[8];
  std::snprintf(buf, sizeof(buf), "%02d", i);
  return buf;
}

void draw_frustum_3d(const pose::Extrinsics& ext, Color color) {
  const glm::dvec3 c = pose::camera_center(ext);
  const glm::dmat3 Rt = glm::transpose(ext.rotation);
  const Vector3 eye{(float)c.x, (float)c.y, (float)c.z};
  // Corner rays of a 1000x1000 image at f = 1148.6, walked 3 m into the scene.
  for (double sx : {-0.435, 0.435})
    for (double sy : {-0.435, 0.435}) {
      const glm::dvec3 dir = Rt * glm::dvec3{sx, sy, 1.0};
      const glm::dvec3 tip = c + glm::normalize(dir) * 3000.0;
      DrawLine3D(eye, Vector3{(float)tip.x, (float)tip.y, (float)tip.z}, color);
    }
}

}  // namespace

int main(int argc, char** argv) try {
  std::filesystem::path data = "data/Pose";
  for (int i = 1; i < argc; ++i)
    if (std::string(argv[i]) == "--data" && i + 1 < argc) data = argv[++i];

  const auto frames = pose::load_poses(data / "poses.txt");
  const double focal = pose::load_focal(data / "focal.txt");
  const auto cameras = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");

  InitWindow(kWindow, kWindow, "pose-explorer");
  if (!IsWindowReady()) {
    std::cerr << "no display available; use ./build/pose-project for the batch pipeline\n";
    return 1;
  }
  SetTargetFPS(60);

  float frame_slider = 0.0f;
  float opacity = 1.0f;
  bool show_3d = false;
  bool photo_background = true;
  bool mode_gt = true;

  Camera3D orbit{};
  orbit.position = {6000.0f, -6000.0f, 3000.0f};
  orbit.target = {0.0f, 0.0f, 500.0f};
  orbit.up = {0.0f, 0.0f, 1.0f};
  orbit.fovy = 45.0f;
  orbit.projection = CAMERA_PERSPECTIVE;

  Texture2D photo{};
  int loaded_photo = -1;

  while (!WindowShouldClose()) {
    const int f = static_cast<int>(frame_slider);
    const auto la = pose::project_frame(frames[f], pose::Mode::LookAt, cameras, focal);
    const auto gt = pose::project_frame(frames[f], pose::Mode::Gt, cameras, focal);
    const auto err = pose::joint_error(la, gt);
    const auto* cam = pose::identify(frames[f].camera_position, cameras, 1.0);

    if (!show_3d && photo_background && loaded_photo != f) {
      if (loaded_photo >= 0) UnloadTexture(photo);
      photo = LoadTexture((data / "frames" / (frame_name(f) + ".png")).string().c_str());
      loaded_photo = f;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (show_3d) {
      UpdateCamera(&orbit, CAMERA_ORBITAL);
      BeginMode3D(orbit);
      DrawGrid(20, 500.0f);
      for (const auto& b : pose::bones()) {
        const auto& p = frames[f].joints[b.a];
        const auto& q = frames[f].joints[b.b];
        DrawLine3D(Vector3{(float)p.x, (float)p.y, (float)p.z},
                   Vector3{(float)q.x, (float)q.y, (float)q.z}, BLACK);
      }
      if (cam != nullptr) draw_frustum_3d(cam->extrinsics, RED);   // published orientation
      draw_frustum_3d(pose::look_at_extrinsics(frames[f].camera_position,
                                               pose::centroid(frames[f]), {0, 0, 1}), BLUE);
      EndMode3D();
    } else {
      if (photo_background && loaded_photo == f)
        DrawTexture(photo, 0, 0, Fade(WHITE, opacity));
      pose::draw_skeleton_2d(mode_gt ? gt : la, 4.0f, 1.0f);
    }

    // Debug panel: buttons, text, sliders. Deliberately plain.
    DrawRectangle(0, 0, 320, 210, Fade(RAYWHITE, 0.9f));
    GuiSliderBar(Rectangle{90, 10, 200, 20}, "frame",
                 TextFormat("%d/%d", f, (int)frames.size() - 1), &frame_slider, 0.0f,
                 (float)frames.size() - 1.0f);
    GuiSliderBar(Rectangle{90, 35, 200, 20}, "opacity", nullptr, &opacity, 0.0f, 1.0f);
    if (GuiButton(Rectangle{10, 65, 140, 24}, show_3d ? "view: 3D" : "view: 2D")) show_3d = !show_3d;
    if (GuiButton(Rectangle{160, 65, 140, 24}, mode_gt ? "proj: gt" : "proj: look-at"))
      mode_gt = !mode_gt;
    if (GuiButton(Rectangle{10, 95, 140, 24}, photo_background ? "bg: photo" : "bg: white"))
      photo_background = !photo_background;
    if (GuiButton(Rectangle{160, 95, 140, 24}, "export figure")) {
      std::filesystem::create_directories("out/figures");
      TakeScreenshot(("out/figures/explorer-" + frame_name(f) + ".png").c_str());
    }
    DrawText(TextFormat("camera %s", cam ? cam->id.c_str() : "unknown"), 10, 130, 18, DARKGRAY);
    DrawText(TextFormat("mean err %.1f px", err.mean_px), 10, 152, 18, DARKGRAY);
    DrawText(TextFormat("worst joint %d @ %.1f px", err.worst_joint, err.max_px), 10, 174, 18,
             DARKGRAY);

    EndDrawing();
  }

  if (loaded_photo >= 0) UnloadTexture(photo);
  CloseWindow();
  return 0;
} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << "\n";
  return 1;
}
```

- [ ] **Step 2: Add the binary to CMake**

```cmake
add_executable(pose-explorer src/main_explorer.cpp)
target_link_libraries(pose-explorer PRIVATE pose_core)
```

- [ ] **Step 3: Build**

Run: `cmake -S . -B build && cmake --build build -j`
Expected: builds clean. `raygui.h` comes from the include directory added in Task 1.

- [ ] **Step 4: Run it (needs a display)**

Run: `./build/pose-explorer --data data/Pose`
Expected: a window showing frame 0's skeleton over the photograph. Drag the frame slider through all 20; toggle to `view: 3D` and confirm the red (published) and blue (look-at) frusta visibly diverge from the same apex. Press `export figure` once and confirm `out/figures/explorer-00.png` exists.

- [ ] **Step 5: Confirm the batch tool still works headless**

Run: `xvfb-run -a ./build/pose-project --data data/Pose --out out --mode both`
Expected: succeeds — proving the deliverable does not depend on the explorer or a display.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: plain raylib debug explorer with 2D and 3D views"
```

---

### Task 11: The LaTeX write-up and final verification

**Files:**
- Create: `report/report.tex`
- Modify: `CMakeLists.txt`, `README.md`

**Interfaces:**
- Consumes: everything in `out/`.
- Produces: `report/report.pdf` — the graded deliverable.

- [ ] **Step 1: Install LaTeX**

Run: `sudo pacman -S --needed texlive-latex texlive-latexrecommended texlive-fontsrecommended texlive-binextra`
Then: `pdflatex --version | head -1`
Expected: a version line.

- [ ] **Step 2: Write `report/report.tex`**

```latex
\documentclass[11pt]{article}
\usepackage[margin=1in]{geometry}
\usepackage{graphicx,booktabs,longtable,hyperref,subcaption}
\graphicspath{{../out/}}

\title{Projecting 3D Human Poses onto 2D Images}
\author{Jeremy}
\date{\today}

\begin{document}
\maketitle

\section{The task and the data}
% What the 45 columns are; 20 frames; focal 1148.6; 14 joints; units in mm.

\section{Identifying the cameras}
% The 20 rows use only 4 distinct camera positions, 5 frames each.

\section{(a) Choosing a camera orientation}
% Look-at construction; why the roll is a free choice that must be justified.

\section{(b) Projection and the 20 renders}
\begin{figure}[htbp]\centering
  \includegraphics[width=0.45\textwidth]{white/00.png}
  \includegraphics[width=0.45\textwidth]{white/01.png}
  \caption{Projected skeletons on a white background (frames 00 and 01).}
\end{figure}

\section{(c) Superimposing on the photographs}
\begin{figure}[htbp]\centering
  \includegraphics[width=0.9\textwidth]{panel/00.png}
  \caption{Overlay on the original frame beside the white-background projection.}
\end{figure}

\section{Error analysis}
% Look-at vs published calibration; the principal-point artefact; one focal for four cameras.

\section{The consolidated coordinate table}
\input{../out/coords/all-2d-coordinates.tex}

\section{Limitations and references}
\begin{thebibliography}{9}
\bibitem{h36m} Ionescu et al., \emph{Human3.6M}, IEEE TPAMI 2014.
\bibitem{calib} K.\ Iskakov, \emph{human36m-camera-parameters}, MIT licence,
  \url{https://github.com/karfly/human36m-camera-parameters}.
\bibitem{raylib} R.\ Santamaria, \emph{raylib}, \url{https://www.raylib.com}.
\end{thebibliography}

\end{document}
```

- [ ] **Step 3: Build the PDF**

Run: `cd report && pdflatex -interaction=nonstopmode report.tex && pdflatex -interaction=nonstopmode report.tex && cd ..`
Expected: `report/report.pdf` exists. The second pass resolves the `longtable` column widths.

- [ ] **Step 4: Add a CMake convenience target**

Append to `CMakeLists.txt`:

```cmake
find_program(PDFLATEX_EXECUTABLE pdflatex)
if(PDFLATEX_EXECUTABLE)
  add_custom_target(report
    COMMAND ${PDFLATEX_EXECUTABLE} -interaction=nonstopmode report.tex
    COMMAND ${PDFLATEX_EXECUTABLE} -interaction=nonstopmode report.tex
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/report
    COMMENT "Building the write-up PDF")
endif()
```

Run: `cmake -S . -B build && cmake --build build --target report`
Expected: rebuilds the PDF.

- [ ] **Step 5: Write the prose**

Fill in every commented section of `report.tex` with the actual analysis: what the data is, how the four cameras were identified, how the look-at orientation is constructed and why its roll choice is arbitrary, the projection equations, the measured look-at-vs-truth error from `out/analysis/joint-error.csv`, and the limitation that `focal.txt` supplies one camera's focal for all four. Include the numbers the program actually produced — never a remembered value.

- [ ] **Step 6: Full clean-room verification**

Run:
```bash
rm -rf build out
cmake -S . -B build && cmake --build build -j
./build/pose_tests
xvfb-run -a ./build/pose-project --data data/Pose --out out --mode both
wc -l out/coords/all-2d-coordinates.csv
cmake --build build --target report && ls -la report/report.pdf
```
Expected: tests pass, 281 CSV lines, PDF exists. Everything reproducible from a clean tree.

- [ ] **Step 7: Update `README.md` with a Results section**

Add the measured mean and worst-case look-at error, and a line stating that `report/report.pdf` is the deliverable.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "docs: LaTeX write-up, report build target and results summary"
```

---

## Self-Review

**Spec coverage:**

| Spec section | Task |
| --- | --- |
| §1 (a) camera orientation | 4 |
| §1 (b) white-background renders | 7 |
| §1 consolidated table | 9 |
| §1 (c) overlays | 8 |
| §2 data facts (20×45, 14 joints, 4 cameras) | 2, 5 |
| §3 vendored calibration, pinned libraries | 1 |
| §4 both projection modes and their comparison | 5, 9 |
| §5 `pose_io` / `skeleton` / `camera` / `analysis` / `draw` / `render` / mains | 2, 3, 4+5, 6, 7, 7+8, 7+10 |
| §5 explorer, one view, plain panel | 10 |
| §6 output layout | 7, 8, 9, 10 |
| §7 error handling (malformed input, z ≤ 0, missing frame, unmatched camera, no GL) | 2, 4, 8, 7, 7 |
| §8 testing | 2, 3, 4, 5, 6 |
| §9 write-up outline | 11 |
| §10 texlive, visibility | 11, outside code |

**Placeholder scan:** the only `%` comment placeholders are the LaTeX section stubs, which Task 11 Step 5 explicitly fills with measured numbers. No `TBD`/`TODO` remains.

**Type consistency:** `Frame`, `Extrinsics`, `Intrinsics`, `CalibratedCamera`, `Mode`, `FrameError` are defined once and used with the same names and signatures throughout. `project_frame`, `identify`, `look_at_extrinsics`, `centroid` and `camera_center` keep consistent parameter orders across Tasks 4–10.

**Known risk flagged for the executor:** Task 8 Step 5 is a hard gate. If the overlay does not land on the subject, the rotation convention is wrong and every downstream number is invalid — debug there rather than proceeding.
