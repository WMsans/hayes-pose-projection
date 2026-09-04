#include "draw.hpp"

#include <raylib.h>

#include <cmath>
#include <cstddef>

#include "render.hpp"
#include "skeleton.hpp"

namespace pose {
namespace {

unsigned char alpha_byte(float alpha) {
  if (!std::isfinite(alpha) || alpha <= 0.0f) return 0;
  if (alpha >= 1.0f) return 255;
  return static_cast<unsigned char>(alpha * 255.0f);
}

bool valid_point(const glm::dvec2& point) {
  return std::isfinite(point.x) && std::isfinite(point.y);
}

Color side_color(Side side, float alpha) {
  const unsigned char a = alpha_byte(alpha);
  switch (side) {
    case Side::Left:  return Color{40, 90, 220, a};
    case Side::Right: return Color{220, 50, 50, a};
    default:          return Color{20, 20, 20, a};
  }
}

void draw_skeleton_2d_impl(
    const std::vector<glm::dvec2>& uv,
    const std::array<JointProjectionStatus, kJointCount>* statuses, float thickness, float alpha) {
  const auto point_is_valid = [&](int index) {
    return index >= 0 && static_cast<std::size_t>(index) < uv.size() &&
           (statuses == nullptr ||
            (static_cast<std::size_t>(index) < kJointCount &&
             (*statuses)[static_cast<std::size_t>(index)] == JointProjectionStatus::Valid)) &&
           valid_point(uv[static_cast<std::size_t>(index)]);
  };

  for (const auto& b : bones()) {
    if (!point_is_valid(b.a) || !point_is_valid(b.b)) continue;

    const Vector2 p{static_cast<float>(uv[b.a].x), static_cast<float>(uv[b.a].y)};
    const Vector2 q{static_cast<float>(uv[b.b].x), static_cast<float>(uv[b.b].y)};
    DrawLineEx(p, q, thickness, side_color(b.side, alpha));
  }
  for (std::size_t i = 0; i < uv.size(); ++i) {
    if (!point_is_valid(static_cast<int>(i))) continue;
    const auto& p = uv[i];
    DrawCircle(static_cast<int>(p.x), static_cast<int>(p.y), thickness * 0.9f,
               Color{20, 20, 20, alpha_byte(alpha)});
  }
}

}  // namespace

void draw_skeleton_2d(const std::vector<glm::dvec2>& uv, float thickness, float alpha) {
  draw_skeleton_2d_impl(uv, nullptr, thickness, alpha);
}

void draw_skeleton_2d(const ProjectedFrame& projected, float thickness, float alpha) {
  draw_skeleton_2d_impl(projected.uv, &projected.joint_status, thickness, alpha);
}

}  // namespace pose
