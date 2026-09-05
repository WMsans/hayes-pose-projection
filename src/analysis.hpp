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
