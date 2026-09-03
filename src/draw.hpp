#pragma once
#include <glm/vec2.hpp>
#include <vector>

namespace pose {

// Colours follow the challenge page's sample figure: right limbs red,
// left limbs blue, torso black. Call inside an active raylib draw scope.
void draw_skeleton_2d(const std::vector<glm::dvec2>& uv, float thickness, float alpha);

}  // namespace pose
