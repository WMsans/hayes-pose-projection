#include "render.hpp"

#include <raylib.h>

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

std::vector<glm::dvec2> project_frame(const Frame& frame, Mode mode,
                                      const std::vector<CalibratedCamera>& cameras, double focal) {
  std::vector<glm::dvec2> uv;
  uv.reserve(kJointCount);

  if (mode == Mode::LookAt) {
    const auto ext = look_at_extrinsics(frame.camera_position, centroid(frame), {0.0, 0.0, 1.0});
    const auto k = challenge_intrinsics(focal);
    for (const auto& j : frame.joints) uv.push_back(project(j, ext, k, false));
    return uv;
  }

  const auto* cam = identify(frame.camera_position, cameras, 1.0);
  if (cam == nullptr)
    throw std::runtime_error("no published camera matches this position; use --mode lookat");
  for (const auto& j : frame.joints) uv.push_back(project(j, cam->extrinsics, cam->intrinsics, false));
  return uv;
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

}  // namespace pose
