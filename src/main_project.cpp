#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "analysis.hpp"
#include "render.hpp"

namespace {

class OffscreenScope {
 public:
  OffscreenScope() {
    pose::begin_offscreen();
    active_ = true;
  }
  ~OffscreenScope() noexcept {
    if (active_) pose::end_offscreen();
  }

  OffscreenScope(const OffscreenScope&) = delete;
  OffscreenScope& operator=(const OffscreenScope&) = delete;

 private:
  bool active_{false};
};

struct Options {
  std::filesystem::path data{"data/Pose"};
  std::filesystem::path out{"out"};
  std::string mode{"both"};
};

Options parse_args(int argc, char** argv) {
  Options o;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const bool has_next = i + 1 < argc;
    if (a == "--data" && has_next) o.data = argv[++i];
    else if (a == "--out" && has_next) o.out = argv[++i];
    else if (a == "--mode" && has_next) o.mode = argv[++i];
    else throw std::runtime_error("usage: pose-project --data <dir> --out <dir> --mode lookat|gt|both");
  }
  if (o.mode != "lookat" && o.mode != "gt" && o.mode != "both")
    throw std::runtime_error("--mode must be lookat, gt or both");
  return o;
}

std::string frame_name(std::size_t i) {
  char buf[8];
  std::snprintf(buf, sizeof(buf), "%02zu", i);
  return buf;
}

}  // namespace

int main(int argc, char** argv) try {
  const Options opt = parse_args(argc, argv);

  const auto frames = pose::load_poses(opt.data / "poses.txt");
  const double focal = pose::load_focal(opt.data / "focal.txt");
  std::vector<pose::CalibratedCamera> cameras;
  if (opt.mode != "lookat")
    cameras = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");

  OffscreenScope offscreen;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const std::string name = frame_name(i);
    const auto mode = (opt.mode == "gt") ? pose::Mode::Gt : pose::Mode::LookAt;
    const auto uv = pose::project_frame(frames[i], mode, cameras, focal);

    const auto white_png = opt.out / "white" / (name + ".png");
    const auto overlay_png = opt.out / "overlay" / (name + ".png");
    pose::render_white(uv, white_png);

    // The overlay is only meaningful with the published calibration; a look-at
    // overlay would place the skeleton at the image centre regardless of the photo.
    const auto overlay_uv =
        (opt.mode == "lookat") ? uv : pose::project_frame(frames[i], pose::Mode::Gt, cameras, focal);
    pose::render_overlay(overlay_uv, opt.data / "frames" / (name + ".png"), overlay_png);
    pose::render_panel(overlay_png, white_png, opt.out / "panel" / (name + ".png"));
  }
  std::cout << "wrote " << frames.size() << " white renders to " << (opt.out / "white") << "\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << "\n";
  return 1;
}
