#include <doctest/doctest.h>
#include "pose_io.hpp"
#include <fstream>
#include <sstream>

namespace {

std::string pose_row(std::size_t columns = pose::kPoseColumns) {
  std::ostringstream out;
  for (std::size_t i = 0; i < columns; ++i) {
    if (i != 0) out << ' ';
    out << 0;
  }
  return out.str();
}

}  // namespace

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

TEST_CASE("load_poses rejects nonnumeric tokens and extra columns") {
  std::ofstream("build/nonnumeric_poses.txt") << "0 0 nope" << pose_row().substr(6) << "\n";
  CHECK_THROWS_AS(pose::load_poses("build/nonnumeric_poses.txt"), std::runtime_error);

  std::ofstream("build/extra_column_poses.txt") << pose_row(pose::kPoseColumns + 1) << "\n";
  CHECK_THROWS_AS(pose::load_poses("build/extra_column_poses.txt"), std::runtime_error);
}

TEST_CASE("load_poses rejects an empty file") {
  std::ofstream("build/empty_poses.txt");
  CHECK_THROWS_AS(pose::load_poses("build/empty_poses.txt"), std::runtime_error);
}

TEST_CASE("load_focal requires one positive finite value") {
  std::ofstream("build/invalid_focal.txt") << "not-a-number\n";
  CHECK_THROWS_AS(pose::load_focal("build/invalid_focal.txt"), std::runtime_error);

  std::ofstream("build/zero_focal.txt") << "0\n";
  CHECK_THROWS_AS(pose::load_focal("build/zero_focal.txt"), std::runtime_error);

  std::ofstream("build/negative_focal.txt") << "-1\n";
  CHECK_THROWS_AS(pose::load_focal("build/negative_focal.txt"), std::runtime_error);

  std::ofstream("build/nan_focal.txt") << "nan\n";
  CHECK_THROWS_AS(pose::load_focal("build/nan_focal.txt"), std::runtime_error);

  std::ofstream("build/empty_focal.txt");
  CHECK_THROWS_AS(pose::load_focal("build/empty_focal.txt"), std::runtime_error);
}

TEST_CASE("load_focal rejects trailing data") {
  std::ofstream("build/trailing_focal.txt") << "1148.6 garbage\n";
  CHECK_THROWS_AS(pose::load_focal("build/trailing_focal.txt"), std::runtime_error);
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

TEST_CASE("load_joint_names rejects malformed records") {
  std::ofstream("build/malformed_joints.txt") << "0   'Hip'\nmalformed\n";
  CHECK_THROWS_AS(pose::load_joint_names("build/malformed_joints.txt"), std::runtime_error);

  std::ofstream("build/empty_name_joints.txt") << "0   ''\n";
  CHECK_THROWS_AS(pose::load_joint_names("build/empty_name_joints.txt"), std::runtime_error);

  std::ofstream("build/trailing_joints.txt") << "0   'Hip' extra\n";
  CHECK_THROWS_AS(pose::load_joint_names("build/trailing_joints.txt"), std::runtime_error);
}
