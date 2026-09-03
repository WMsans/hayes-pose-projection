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
