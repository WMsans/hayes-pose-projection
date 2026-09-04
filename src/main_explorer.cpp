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

  InitWindow(kWindow, kWindow, "pose-explorer");
  if (!IsWindowReady()) {
    std::cerr << "no display available; use ./build/pose-project for the batch pipeline\n";
    return 1;
  }
  SetTargetFPS(60);

  float frame_slider = 0.0f;
  float opacity = 1.0f;
  bool show_3d = false;
  bool photo_background = true;
  bool mode_gt = true;

  Camera3D orbit{};
  orbit.position = {6000.0f, -6000.0f, 3000.0f};
  orbit.target = {0.0f, 0.0f, 500.0f};
  orbit.up = {0.0f, 0.0f, 1.0f};
  orbit.fovy = 45.0f;
  orbit.projection = CAMERA_PERSPECTIVE;

  Texture2D photo{};
  int loaded_photo = -1;

  while (!WindowShouldClose()) {
    const int f = static_cast<int>(frame_slider);
    const auto la = pose::project_frame(frames[f], pose::Mode::LookAt, cameras, focal);
    const auto gt = pose::project_frame(frames[f], pose::Mode::Gt, cameras, focal);
    const auto err = pose::joint_error(la, gt);
    const auto* cam = pose::identify(frames[f].camera_position, cameras, 1.0);

    if (!show_3d && photo_background && loaded_photo != f) {
      if (loaded_photo >= 0) UnloadTexture(photo);
      photo = LoadTexture((data / "frames" / (frame_name(f) + ".png")).string().c_str());
      loaded_photo = f;
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
        DrawTexture(photo, 0, 0, Fade(WHITE, opacity));
      pose::draw_skeleton_2d(mode_gt ? gt : la, 4.0f, 1.0f);
    }

    // Debug panel: buttons, text, sliders. Deliberately plain.
    DrawRectangle(0, 0, 320, 210, Fade(RAYWHITE, 0.9f));
    GuiSliderBar(Rectangle{90, 10, 200, 20}, "frame",
                 TextFormat("%d/%d", f, (int)frames.size() - 1), &frame_slider, 0.0f,
                 (float)frames.size() - 1.0f);
    GuiSliderBar(Rectangle{90, 35, 200, 20}, "opacity", nullptr, &opacity, 0.0f, 1.0f);
    if (GuiButton(Rectangle{10, 65, 140, 24}, show_3d ? "view: 3D" : "view: 2D")) show_3d = !show_3d;
    if (GuiButton(Rectangle{160, 65, 140, 24}, mode_gt ? "proj: gt" : "proj: look-at"))
      mode_gt = !mode_gt;
    if (GuiButton(Rectangle{10, 95, 140, 24}, photo_background ? "bg: photo" : "bg: white"))
      photo_background = !photo_background;
    if (GuiButton(Rectangle{160, 95, 140, 24}, "export figure")) {
      std::filesystem::create_directories("out/figures");
      TakeScreenshot(("out/figures/explorer-" + frame_name(f) + ".png").c_str());
    }
    DrawText(TextFormat("camera %s", cam ? cam->id.c_str() : "unknown"), 10, 130, 18, DARKGRAY);
    DrawText(TextFormat("mean err %.1f px", err.mean_px), 10, 152, 18, DARKGRAY);
    DrawText(TextFormat("worst joint %d @ %.1f px", err.worst_joint, err.max_px), 10, 174, 18,
             DARKGRAY);

    EndDrawing();
  }

  if (loaded_photo >= 0) UnloadTexture(photo);
  CloseWindow();
  return 0;
} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << "\n";
  return 1;
}
