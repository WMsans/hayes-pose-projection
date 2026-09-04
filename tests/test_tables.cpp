#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "pose_io.hpp"
#include "tables.hpp"

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream contents;
  contents << in.rdbuf();
  return contents.str();
}

std::filesystem::path test_output_directory() {
  const auto directory = std::filesystem::temp_directory_path() / "hayes-pose-table-tests";
  std::error_code error;
  std::filesystem::remove_all(directory, error);
  std::filesystem::create_directories(directory);
  return directory;
}

std::vector<std::string> joint_names() {
  std::vector<std::string> names;
  for (int i = 0; i < pose::kJointCount; ++i) names.push_back("joint-" + std::to_string(i));
  return names;
}

std::vector<glm::dvec2> coordinates(double offset) {
  std::vector<glm::dvec2> values;
  for (int i = 0; i < pose::kJointCount; ++i)
    values.emplace_back(offset + i + 0.126, offset - i - 0.874);
  return values;
}

}  // namespace

TEST_CASE("write_coordinate_table emits fixed look-at and ground-truth coordinates") {
  const auto directory = test_output_directory();
  pose::write_coordinate_table(directory / "coords.csv", directory / "coords.tex",
                               {coordinates(1.0)}, {coordinates(2.0)}, joint_names(), {"camera-a"});

  const auto csv = read_file(directory / "coords.csv");
  CHECK(csv.starts_with("frame,camera,joint,name,u_lookat,v_lookat,u_gt,v_gt\n"));
  CHECK(csv.find("0,camera-a,0,joint-0,1.13,0.13,2.13,1.13\n") != std::string::npos);
  CHECK(csv.find("0,camera-a,1,joint-1,2.13,-0.87,3.13,0.13\n") != std::string::npos);
  CHECK(std::count(csv.begin(), csv.end(), '\n') == pose::kJointCount + 1);
  CHECK(read_file(directory / "coords.tex").find("$u_{\\text{look-at}}$") != std::string::npos);
  std::error_code error;
  std::filesystem::remove_all(directory, error);
}

TEST_CASE("write_error_tables emits joint and camera angular CSVs") {
  const auto directory = test_output_directory();
  auto lookat = std::vector<std::vector<glm::dvec2>>{coordinates(0.0)};
  auto gt = lookat;
  gt[0][3].x += 4.0;
  pose::write_error_tables(directory, lookat, gt, {12.34567}, {"camera-a"});

  CHECK(read_file(directory / "joint-error.csv") ==
        "frame,camera,mean_px,max_px,worst_joint,angular_deg\n"
        "0,camera-a,0.29,4.00,3,12.3457\n");
  CHECK(read_file(directory / "camera-angular-error.csv") ==
        "frame,camera,angular_deg\n"
        "0,camera-a,12.3457\n");
  std::error_code error;
  std::filesystem::remove_all(directory, error);
}

TEST_CASE("write_error_tables rejects frames without exactly 14 joints") {
  const auto directory = test_output_directory();
  for (const auto joint_count : {1, 13, 15}) {
    const std::vector<glm::dvec2> malformed(joint_count, {1.0, 2.0});
    CHECK_THROWS_WITH(pose::write_error_tables(directory, {malformed}, {malformed}, {0.0},
                                                {"camera-a"}),
                      "write_error_tables: joint count mismatch");
  }
  std::error_code error;
  std::filesystem::remove_all(directory, error);
}
