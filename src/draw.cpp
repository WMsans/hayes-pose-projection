#include "draw.hpp"

#include <raylib.h>

#include <cmath>
#include <cstddef>

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

}  // namespace

void draw_skeleton_2d(const std::vector<glm::dvec2>& uv, float thickness, float alpha) {
  for (const auto& b : bones()) {
    if (b.a < 0 || b.b < 0 || static_cast<std::size_t>(b.a) >= uv.size() ||
        static_cast<std::size_t>(b.b) >= uv.size() || !valid_point(uv[b.a]) ||
        !valid_point(uv[b.b]))
      continue;

    const Vector2 p{static_cast<float>(uv[b.a].x), static_cast<float>(uv[b.a].y)};
    const Vector2 q{static_cast<float>(uv[b.b].x), static_cast<float>(uv[b.b].y)};
    DrawLineEx(p, q, thickness, side_color(b.side, alpha));
  }
  for (const auto& p : uv) {
    if (!valid_point(p)) continue;
    DrawCircle(static_cast<int>(p.x), static_cast<int>(p.y), thickness * 0.9f,
               Color{20, 20, 20, alpha_byte(alpha)});
  }
}

}  // namespace pose
