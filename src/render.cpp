#include "render.hpp"

#include <raylib.h>

#include <stdexcept>

#include "draw.hpp"

namespace pose {

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
  if (!IsWindowReady())
    throw std::runtime_error("no GL context: re-run under 'xvfb-run -a ./build/pose-project ...'");
}

void end_offscreen() { CloseWindow(); }

void render_white(const std::vector<glm::dvec2>& uv, const std::filesystem::path& out_png) {
  if (!out_png.parent_path().empty()) std::filesystem::create_directories(out_png.parent_path());
  RenderTexture2D target = LoadRenderTexture(kImageSize, kImageSize);
  BeginTextureMode(target);
  ClearBackground(RAYWHITE);
  draw_skeleton_2d(uv, 4.0f, 1.0f);
  EndTextureMode();

  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);  // render textures are bottom-up
  ExportImage(img, out_png.string().c_str());
  UnloadImage(img);
  UnloadRenderTexture(target);
}

}  // namespace pose
