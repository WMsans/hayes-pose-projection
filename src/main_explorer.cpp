#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <cstdio>
#include <filesystem>
#include <glm/geometric.hpp>
#include <iostream>
#include <string>

#include "analysis.hpp"
#include "draw.hpp"
#include "render.hpp"
#include "skeleton.hpp"

namespace {

constexpr int kWindow = 1000;

enum class ProjectionView { LookAt, Gt, Both };

const char* projection_label(ProjectionView view) {
  switch (view) {
    case ProjectionView::LookAt: return "lookat";
    case ProjectionView::Gt: return "gt";
    case ProjectionView::Both: return "both";
  }
  return "lookat";
}

class WindowScope {
 public:
  WindowScope(int width, int height, const char* title) : active_(true) {
    InitWindow(width, height, title);
  }
  ~WindowScope() {
    if (active_) CloseWindow();
  }

  WindowScope(const WindowScope&) = delete;
  WindowScope& operator=(const WindowScope&) = delete;

  bool ready() const { return IsWindowReady(); }

 private:
  bool active_;
};

class PhotoScope {
 public:
  PhotoScope() = default;
  ~PhotoScope() {
    if (IsTextureValid(texture_)) UnloadTexture(texture_);
  }

  PhotoScope(const PhotoScope&) = delete;
  PhotoScope& operator=(const PhotoScope&) = delete;

  void reset(Texture2D texture) {
    if (IsTextureValid(texture_)) UnloadTexture(texture_);
    texture_ = texture;
  }

  const Texture2D& get() const { return texture_; }

 private:
  Texture2D texture_{};
};

std::string frame_name(int i) {
  char buf[8];
  std::snprintf(buf, sizeof(buf), "%02d", i);
  return buf;
}

void draw_frustum_3d(const pose::Extrinsics& ext, Color color) {
  const glm::dvec3 c = pose::camera_center(ext);
  const glm::dmat3 Rt = glm::transpose(ext.rotation);
  const Vector3 eye{(float)c.x, (float)c.y, (float)c.z};
  // Corner rays of a 1000x1000 image at f = 1148.6, walked 3 m into the scene.
  for (double sx : {-0.435, 0.435})
    for (double sy : {-0.435, 0.435}) {
      const glm::dvec3 dir = Rt * glm::dvec3{sx, sy, 1.0};
      const glm::dvec3 tip = c + glm::normalize(dir) * 3000.0;
      DrawLine3D(eye, Vector3{(float)tip.x, (float)tip.y, (float)tip.z}, color);
    }
}

}  // namespace

int main(int argc, char** argv) try {
  std::filesystem::path data = "data/Pose";
  for (int i = 1; i < argc; ++i)
    if (std::string(argv[i]) == "--data" && i + 1 < argc) data = argv[++i];

  const auto frames = pose::load_poses(data / "poses.txt");
  const double focal = pose::load_focal(data / "focal.txt");
  const auto cameras = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");

  WindowScope window(kWindow, kWindow, "pose-explorer");
  if (!window.ready()) {
    std::cerr << "no display available; use ./build/pose-project for the batch pipeline\n";
    return 1;
  }
  SetTargetFPS(60);

  float frame_slider = 0.0f;
  float opacity = 1.0f;
  bool show_3d = false;
  bool photo_background = true;
  bool apply_distortion = true;
  ProjectionView projection_mode = ProjectionView::Gt;

  Camera3D orbit{};
  orbit.position = {6000.0f, -6000.0f, 3000.0f};
  orbit.target = {0.0f, 0.0f, 500.0f};
  orbit.up = {0.0f, 0.0f, 1.0f};
  orbit.fovy = 45.0f;
  orbit.projection = CAMERA_PERSPECTIVE;

  PhotoScope photo;
  int loaded_photo = -1;

  while (!WindowShouldClose()) {
    const int f = static_cast<int>(frame_slider);
    const auto la = pose::project_frame(frames[f], pose::Mode::LookAt, cameras, focal, false);
    const auto gt = pose::project_frame(frames[f], pose::Mode::Gt, cameras, focal, apply_distortion);
    const auto err = pose::joint_error(la, gt);
    const auto* cam = pose::identify(frames[f].camera_position, cameras, 1.0);

    if (!show_3d && photo_background && loaded_photo != f) {
      const auto frame_path = data / "frames" / (frame_name(f) + ".png");
      const Texture2D loaded = LoadTexture(frame_path.string().c_str());
      if (IsTextureValid(loaded)) {
        photo.reset(loaded);
        loaded_photo = f;
      } else {
        TraceLog(LOG_WARNING, "failed to load frame %s", frame_path.string().c_str());
        loaded_photo = -1;
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (show_3d) {
      UpdateCamera(&orbit, CAMERA_ORBITAL);
      BeginMode3D(orbit);
      DrawGrid(20, 500.0f);
      for (const auto& b : pose::bones()) {
        const auto& p = frames[f].joints[b.a];
        const auto& q = frames[f].joints[b.b];
        DrawLine3D(Vector3{(float)p.x, (float)p.y, (float)p.z},
                   Vector3{(float)q.x, (float)q.y, (float)q.z}, BLACK);
      }
      if (cam != nullptr) draw_frustum_3d(cam->extrinsics, RED);   // published orientation
      draw_frustum_3d(pose::look_at_extrinsics(frames[f].camera_position,
                                               pose::centroid(frames[f]), {0, 0, 1}), BLUE);
      EndMode3D();
    } else {
      if (photo_background && loaded_photo == f)
        DrawTexture(photo.get(), 0, 0, Fade(WHITE, opacity));
      if (projection_mode == ProjectionView::LookAt)
        pose::draw_skeleton_2d(la, 4.0f, 1.0f);
      else if (projection_mode == ProjectionView::Gt)
        pose::draw_skeleton_2d(gt, 4.0f, 1.0f);
      else {
        pose::draw_skeleton_2d(la, 4.0f, 0.65f);
        pose::draw_skeleton_2d(gt, 4.0f, 0.65f);
      }
    }

    // Debug panel: buttons, text, sliders. Deliberately plain.
    DrawRectangle(0, 0, 320, 240, Fade(RAYWHITE, 0.9f));
    GuiSliderBar(Rectangle{90, 10, 200, 20}, "frame",
                 TextFormat("%d/%d", f, (int)frames.size() - 1), &frame_slider, 0.0f,
                 (float)frames.size() - 1.0f);
    GuiSliderBar(Rectangle{90, 35, 200, 20}, "opacity", nullptr, &opacity, 0.0f, 1.0f);
    if (GuiButton(Rectangle{10, 65, 140, 24}, show_3d ? "view: 3D" : "view: 2D")) show_3d = !show_3d;
    if (GuiButton(Rectangle{160, 65, 140, 24}, TextFormat("proj: %s", projection_label(projection_mode)))) {
      switch (projection_mode) {
        case ProjectionView::LookAt: projection_mode = ProjectionView::Gt; break;
        case ProjectionView::Gt: projection_mode = ProjectionView::Both; break;
        case ProjectionView::Both: projection_mode = ProjectionView::LookAt; break;
      }
    }
    if (GuiButton(Rectangle{10, 95, 140, 24}, photo_background ? "bg: photo" : "bg: white"))
      photo_background = !photo_background;
    if (GuiButton(Rectangle{160, 95, 140, 24}, apply_distortion ? "distortion: on" : "distortion: off"))
      apply_distortion = !apply_distortion;
    if (GuiButton(Rectangle{10, 125, 140, 24}, "export figure")) {
      std::filesystem::create_directories("out/figures");
      TakeScreenshot(("out/figures/explorer-" + frame_name(f) + ".png").c_str());
    }
    DrawText(TextFormat("camera %s", cam ? cam->id.c_str() : "unknown"), 10, 160, 18, DARKGRAY);
    DrawText(TextFormat("mean err %.1f px", err.mean_px), 10, 182, 18, DARKGRAY);
    DrawText(TextFormat("worst joint %d @ %.1f px", err.worst_joint, err.max_px), 10, 204, 18,
             DARKGRAY);

    EndDrawing();
  }

  return 0;
} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << "\n";
  return 1;
}
