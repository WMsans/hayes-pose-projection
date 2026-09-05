#include "render.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include "draw.hpp"

namespace pose {
namespace {

class RenderTextureScope {
 public:
  explicit RenderTextureScope(RenderTexture2D target) : target_(target) {}
  ~RenderTextureScope() {
    if (target_.id != 0) UnloadRenderTexture(target_);
  }

  RenderTextureScope(const RenderTextureScope&) = delete;
  RenderTextureScope& operator=(const RenderTextureScope&) = delete;

  bool ready() const { return IsRenderTextureValid(target_); }
  const RenderTexture2D& get() const { return target_; }

 private:
  RenderTexture2D target_{};
};

class TextureScope {
 public:
  explicit TextureScope(Texture2D texture) : texture_(texture) {}
  ~TextureScope() {
    if (IsTextureValid(texture_)) UnloadTexture(texture_);
  }

  TextureScope(const TextureScope&) = delete;
  TextureScope& operator=(const TextureScope&) = delete;

  bool ready() const { return IsTextureValid(texture_); }
  const Texture2D& get() const { return texture_; }

 private:
  Texture2D texture_{};
};

class TextureModeScope {
 public:
  explicit TextureModeScope(const RenderTexture2D& target) {
    BeginTextureMode(target);
    active_ = true;
  }
  ~TextureModeScope() {
    if (active_) EndTextureMode();
  }

  TextureModeScope(const TextureModeScope&) = delete;
  TextureModeScope& operator=(const TextureModeScope&) = delete;

 private:
  bool active_{false};
};

class ImageScope {
 public:
  explicit ImageScope(Image image) : image_(image) {}
  ~ImageScope() {
    if (image_.data != nullptr) UnloadImage(image_);
  }

  ImageScope(const ImageScope&) = delete;
  ImageScope& operator=(const ImageScope&) = delete;

  bool ready() const { return IsImageValid(image_); }
  Image& get() { return image_; }

 private:
  Image image_{};
};

}  // namespace

ProjectedFrame::ProjectedFrame() : uv(kJointCount, glm::dvec2{0.0}) {
  joint_status.fill(JointProjectionStatus::Valid);
}

int ProjectedFrame::invalid_joint_count() const {
  int count = 0;
  for (const auto status : joint_status)
    if (status != JointProjectionStatus::Valid) ++count;
  return count;
}

bool ProjectedFrame::complete() const {
  return camera_matched && invalid_joint_count() == 0 && uv.size() == kJointCount;
}

ProjectedFrame project_frame_status(const Frame& frame, Mode mode,
                                    const std::vector<CalibratedCamera>& cameras, double focal,
                                    bool apply_distortion) {
  ProjectedFrame result;
  Extrinsics ext;
  Intrinsics intrinsics;
  if (mode == Mode::LookAt) {
    ext = look_at_extrinsics(frame.camera_position, centroid(frame), {0.0, 0.0, 1.0});
    intrinsics = challenge_intrinsics(focal);
  } else {
    const auto* cam = identify(frame.camera_position, cameras, 1.0);
    if (cam == nullptr) {
      result.camera_matched = false;
      result.joint_status.fill(JointProjectionStatus::UnmatchedCamera);
      const double nan = std::numeric_limits<double>::quiet_NaN();
      std::fill(result.uv.begin(), result.uv.end(), glm::dvec2{nan, nan});
      return result;
    }
    ext = cam->extrinsics;
    intrinsics = cam->intrinsics;
  }

  for (std::size_t i = 0; i < frame.joints.size(); ++i) {
    const glm::dvec3 camera_point = ext.rotation * frame.joints[i] + ext.translation;
    if (camera_point.z <= 0.0) {
      result.joint_status[i] = JointProjectionStatus::BehindCamera;
      const double nan = std::numeric_limits<double>::quiet_NaN();
      result.uv[i] = {nan, nan};
      continue;
    }
    result.uv[i] = project(frame.joints[i], ext, intrinsics, apply_distortion);
  }
  return result;
}

std::vector<glm::dvec2> project_frame(const Frame& frame, Mode mode,
                                      const std::vector<CalibratedCamera>& cameras, double focal,
                                      bool apply_distortion) {
  const auto result = project_frame_status(frame, mode, cameras, focal, apply_distortion);
  if (!result.camera_matched)
    throw std::runtime_error("no published camera matches this position; use --mode lookat");
  for (std::size_t i = 0; i < result.joint_status.size(); ++i) {
    if (result.joint_status[i] == JointProjectionStatus::BehindCamera)
      throw std::runtime_error("project_frame: joint " + std::to_string(i) +
                               " is at or behind the image plane");
  }
  return result.uv;
}

void begin_offscreen() {
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_MSAA_4X_HINT);
  InitWindow(kImageSize, kImageSize, "pose-project");
  if (!IsWindowReady()) {
    CloseWindow();
    throw std::runtime_error("no GL context: re-run under 'xvfb-run -a ./build/pose-project ...'");
  }
}

void end_offscreen() { CloseWindow(); }

void render_white(const std::vector<glm::dvec2>& uv, const std::filesystem::path& out_png) {
  if (!out_png.parent_path().empty()) std::filesystem::create_directories(out_png.parent_path());

  RenderTextureScope target(LoadRenderTexture(kImageSize, kImageSize));
  if (!target.ready())
    throw std::runtime_error("failed to allocate render texture");

  {
    TextureModeScope texture_mode(target.get());
    ClearBackground(RAYWHITE);
    draw_skeleton_2d(uv, 4.0f, 1.0f);
  }

  ImageScope image(LoadImageFromTexture(target.get().texture));
  if (!image.ready())
    throw std::runtime_error("failed to read render texture into image");

  ImageFlipVertical(&image.get());  // render textures are bottom-up
  if (!ExportImage(image.get(), out_png.string().c_str()))
    throw std::runtime_error("failed to export render to '" + out_png.string() + "'");
}

void render_overlay(const std::vector<glm::dvec2>& uv, const std::filesystem::path& frame_png,
                    const std::filesystem::path& out_png) {
  if (!std::filesystem::exists(frame_png)) {
    TraceLog(LOG_WARNING, "missing frame %s; skipping overlay", frame_png.string().c_str());
    return;
  }
  if (!out_png.parent_path().empty()) std::filesystem::create_directories(out_png.parent_path());

  TextureScope photo(LoadTexture(frame_png.string().c_str()));
  if (!photo.ready()) throw std::runtime_error("failed to load frame '" + frame_png.string() + "'");

  RenderTextureScope target(LoadRenderTexture(kImageSize, kImageSize));
  if (!target.ready()) throw std::runtime_error("failed to allocate overlay render texture");
  {
    TextureModeScope texture_mode(target.get());
    ClearBackground(BLACK);
    DrawTexture(photo.get(), 0, 0, WHITE);
    draw_skeleton_2d(uv, 4.0f, 1.0f);
  }

  ImageScope image(LoadImageFromTexture(target.get().texture));
  if (!image.ready()) throw std::runtime_error("failed to read overlay render texture into image");
  ImageFlipVertical(&image.get());  // render textures are bottom-up
  if (!ExportImage(image.get(), out_png.string().c_str()))
    throw std::runtime_error("failed to export overlay to '" + out_png.string() + "'");
}

void render_panel(const std::filesystem::path& left_png, const std::filesystem::path& right_png,
                  const std::filesystem::path& out_png) {
  if (!std::filesystem::exists(left_png) || !std::filesystem::exists(right_png)) return;
  if (!out_png.parent_path().empty()) std::filesystem::create_directories(out_png.parent_path());

  ImageScope left(LoadImage(left_png.string().c_str()));
  ImageScope right(LoadImage(right_png.string().c_str()));
  if (!left.ready() || !right.ready())
    throw std::runtime_error("failed to load panel input image");
  if (left.get().width != right.get().width || left.get().height != right.get().height)
    throw std::runtime_error("panel input images must have equal dimensions");

  ImageScope panel(GenImageColor(left.get().width + right.get().width, left.get().height, RAYWHITE));
  if (!panel.ready()) throw std::runtime_error("failed to allocate panel image");
  ImageDraw(&panel.get(), left.get(), Rectangle{0, 0, static_cast<float>(left.get().width),
                                                 static_cast<float>(left.get().height)},
            Rectangle{0, 0, static_cast<float>(left.get().width),
                      static_cast<float>(left.get().height)}, WHITE);
  ImageDraw(&panel.get(), right.get(), Rectangle{0, 0, static_cast<float>(right.get().width),
                                                  static_cast<float>(right.get().height)},
            Rectangle{static_cast<float>(left.get().width), 0,
                      static_cast<float>(right.get().width), static_cast<float>(right.get().height)},
            WHITE);
  if (!ExportImage(panel.get(), out_png.string().c_str()))
    throw std::runtime_error("failed to export panel to '" + out_png.string() + "'");
}

}  // namespace pose
