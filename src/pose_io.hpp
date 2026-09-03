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
