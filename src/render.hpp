#pragma once
#include <array>
#include <filesystem>
#include <glm/vec2.hpp>
#include <vector>

#include "camera.hpp"
#include "pose_io.hpp"

namespace pose {

inline constexpr int kImageSize = 1000;

enum class Mode { LookAt, Gt };

enum class JointProjectionStatus { Valid, BehindCamera, UnmatchedCamera };

struct ProjectedFrame {
  std::vector<glm::dvec2> uv;
  std::array<JointProjectionStatus, kJointCount> joint_status{};
  bool camera_matched{true};

  ProjectedFrame();
  int invalid_joint_count() const;
  bool complete() const;
};

// Reports each joint independently so a batch can continue after a bad depth.
// Unmatched GT cameras produce no coordinates and are marked explicitly.
ProjectedFrame project_frame_status(const Frame& frame, Mode mode,
                                    const std::vector<CalibratedCamera>& cameras, double focal,
                                    bool apply_distortion = false);

// Compatibility wrapper: preserves the historical throwing behavior for callers that require a
// complete projection. Batch code should use project_frame_status instead.
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
