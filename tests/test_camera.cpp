#include <doctest/doctest.h>
#include "camera.hpp"
#include <filesystem>
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

TEST_CASE("a small positive depth is projected") {
  pose::Extrinsics ext{glm::dmat3(1.0), glm::dvec3(0.0)};
  pose::Intrinsics k{1000.0, 1000.0, 500.0, 500.0, {}, false};
  const auto uv = pose::project({0.0, 0.0, 1e-7}, ext, k, false);
  CHECK(uv.x == doctest::Approx(500.0));
  CHECK(uv.y == doctest::Approx(500.0));
}

TEST_CASE("centroid averages all 14 joints") {
  pose::Frame f;
  for (auto& j : f.joints) j = {3.0, 6.0, 9.0};
  const auto c = pose::centroid(f);
  CHECK(c.x == doctest::Approx(3.0));
  CHECK(c.z == doctest::Approx(9.0));
}

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
