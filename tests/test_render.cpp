#include <doctest/doctest.h>

#include <cstddef>
#include <exception>
#include <filesystem>
#include <string>
#include <system_error>

#include <raylib.h>

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

namespace {

class OptionalOffscreen {
 public:
  OptionalOffscreen() {
    try {
      pose::begin_offscreen();
      active_ = true;
    } catch (const std::exception& e) {
      reason_ = e.what();
    }
  }

  ~OptionalOffscreen() {
    if (active_) pose::end_offscreen();
  }

  bool active() const { return active_; }
  const std::string& reason() const { return reason_; }

 private:
  bool active_{false};
  std::string reason_;
};

std::filesystem::path test_output_directory() {
  const auto directory = std::filesystem::temp_directory_path() / "hayes-pose-render-tests";
  std::error_code error;
  std::filesystem::remove_all(directory, error);
  std::filesystem::create_directories(directory);
  return directory;
}

}  // namespace

TEST_CASE("render_white writes a 1000x1000 RAYWHITE image for empty input") {
  OptionalOffscreen offscreen;
  if (!offscreen.active()) {
    WARN("skipping optional render test");
    return;
  }

  const auto directory = test_output_directory();
  const auto output = directory / "white.png";
  pose::render_white({}, output);

  Image image = LoadImage(output.string().c_str());
  REQUIRE(IsImageValid(image));
  CHECK(image.width == pose::kImageSize);
  CHECK(image.height == pose::kImageSize);
  const Color corner = GetImageColor(image, 0, 0);
  CHECK(corner.r == RAYWHITE.r);
  CHECK(corner.g == RAYWHITE.g);
  CHECK(corner.b == RAYWHITE.b);
  CHECK(corner.a == RAYWHITE.a);
  UnloadImage(image);
  std::error_code error;
  std::filesystem::remove_all(directory, error);
}

TEST_CASE("render_white reports an image export failure") {
  OptionalOffscreen offscreen;
  if (!offscreen.active()) {
    WARN("skipping optional render test");
    return;
  }

  const auto directory = test_output_directory();
  const auto output_directory = directory / "not-an-image-file";
  std::filesystem::create_directory(output_directory);
  CHECK_THROWS_WITH(pose::render_white({}, output_directory),
                    doctest::Contains("failed to export render"));
  std::error_code error;
  std::filesystem::remove_all(directory, error);
}

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
