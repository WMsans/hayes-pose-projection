#include "draw.hpp"

#include <raylib.h>

#include "skeleton.hpp"

namespace pose {
namespace {

Color side_color(Side side, float alpha) {
  const unsigned char a = static_cast<unsigned char>(alpha * 255.0f);
  switch (side) {
    case Side::Left:  return Color{40, 90, 220, a};
    case Side::Right: return Color{220, 50, 50, a};
    default:          return Color{20, 20, 20, a};
  }
}

}  // namespace

void draw_skeleton_2d(const std::vector<glm::dvec2>& uv, float thickness, float alpha) {
  for (const auto& b : bones()) {
    const Vector2 p{static_cast<float>(uv[b.a].x), static_cast<float>(uv[b.a].y)};
    const Vector2 q{static_cast<float>(uv[b.b].x), static_cast<float>(uv[b.b].y)};
    DrawLineEx(p, q, thickness, side_color(b.side, alpha));
  }
  for (const auto& p : uv)
    DrawCircle(static_cast<int>(p.x), static_cast<int>(p.y), thickness * 0.9f,
               Color{20, 20, 20, static_cast<unsigned char>(alpha * 255.0f)});
}

}  // namespace pose
