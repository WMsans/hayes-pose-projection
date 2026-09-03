#include <doctest/doctest.h>

#include <cstddef>

#include "render.hpp"

namespace {

pose::Frame centered_frame() {
  pose::Frame frame;
  frame.camera_position = {0.0, -1000.0, 0.0};
  for (auto& joint : frame.joints) joint = {0.0, 0.0, 0.0};
  frame.joints[1].x = 100.0;
  frame.joints[2].x = -100.0;
  return frame;
}

}  // namespace

TEST_CASE("project_frame LookAt projects all joints with challenge intrinsics") {
  const auto uv = pose::project_frame(centered_frame(), pose::Mode::LookAt, {}, 1000.0);

  REQUIRE(uv.size() == pose::kJointCount);
  CHECK(uv[0].x == doctest::Approx(500.0));
  CHECK(uv[0].y == doctest::Approx(500.0));
  CHECK(uv[1].x == doctest::Approx(600.0));
  CHECK(uv[1].y == doctest::Approx(500.0));
}

TEST_CASE("project_frame Gt uses the identified calibrated camera") {
  pose::Frame frame;
  frame.camera_position = {0.0, 0.0, 0.0};
  frame.joints[0] = {0.0, 0.0, 1000.0};
  frame.joints[1] = {100.0, 50.0, 1000.0};

  pose::CalibratedCamera camera;
  camera.id = "test";
  camera.center = frame.camera_position;
  camera.intrinsics.fx = 800.0;
  camera.intrinsics.fy = 600.0;
  camera.intrinsics.cx = 320.0;
  camera.intrinsics.cy = 240.0;
  camera.extrinsics.rotation = glm::dmat3(1.0);
  camera.extrinsics.translation = {0.0, 0.0, 0.0};

  for (std::size_t i = 2; i < frame.joints.size(); ++i)
    frame.joints[i] = {0.0, 0.0, 1000.0};

  const auto uv = pose::project_frame(frame, pose::Mode::Gt, {camera}, 0.0);

  REQUIRE(uv.size() == pose::kJointCount);
  CHECK(uv[1].x == doctest::Approx(400.0));
  CHECK(uv[1].y == doctest::Approx(270.0));
}

TEST_CASE("project_frame Gt rejects an unknown camera position") {
  pose::Frame frame;
  frame.camera_position = {100.0, 0.0, 0.0};
  for (auto& joint : frame.joints) joint = {0.0, 0.0, 1000.0};

  CHECK_THROWS_WITH(pose::project_frame(frame, pose::Mode::Gt, {}, 0.0),
                    "no published camera matches this position; use --mode lookat");
}
