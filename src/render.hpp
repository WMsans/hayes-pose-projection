#pragma once
#include <filesystem>
#include <glm/vec2.hpp>
#include <vector>

#include "camera.hpp"
#include "pose_io.hpp"

namespace pose {

inline constexpr int kImageSize = 1000;

enum class Mode { LookAt, Gt };

// Projects all 14 joints of a frame. LookAt uses only challenge-supplied data;
// Gt uses the published calibration and throws if the camera cannot be identified.
std::vector<glm::dvec2> project_frame(const Frame& frame, Mode mode,
                                      const std::vector<CalibratedCamera>& cameras, double focal,
                                      bool apply_distortion = false);

// Offscreen rendering. Requires a GL context; run under xvfb-run if headless.
void begin_offscreen();
void end_offscreen();
void render_white(const std::vector<glm::dvec2>& uv, const std::filesystem::path& out_png);

// Draws the skeleton over the photograph. Skipped with a warning if the frame is missing.
void render_overlay(const std::vector<glm::dvec2>& uv, const std::filesystem::path& frame_png,
                    const std::filesystem::path& out_png);

// Composites two equally sized PNGs side by side, echoing the challenge page's sample figure.
void render_panel(const std::filesystem::path& left_png, const std::filesystem::path& right_png,
                  const std::filesystem::path& out_png);

}  // namespace pose
