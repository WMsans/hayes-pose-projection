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
