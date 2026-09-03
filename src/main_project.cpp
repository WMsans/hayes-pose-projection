#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "analysis.hpp"
#include "render.hpp"
#include "tables.hpp"

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

  std::vector<std::vector<glm::dvec2>> lookat_uv;
  std::vector<std::vector<glm::dvec2>> gt_uv;
  std::vector<std::string> camera_ids;
  std::vector<double> angular_deg;

  OffscreenScope offscreen;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const std::string name = frame_name(i);
    std::vector<glm::dvec2> uv;
    std::vector<glm::dvec2> overlay_uv;
    if (opt.mode == "lookat") {
      uv = pose::project_frame(frames[i], pose::Mode::LookAt, cameras, focal);
      overlay_uv = uv;
    } else {
      const auto la = pose::project_frame(frames[i], pose::Mode::LookAt, cameras, focal);
      const auto gt = pose::project_frame(frames[i], pose::Mode::Gt, cameras, focal);
      lookat_uv.push_back(la);
      gt_uv.push_back(gt);

      const auto* cam = pose::identify(frames[i].camera_position, cameras, 1.0);
      camera_ids.push_back(cam ? cam->id : "unknown");
      const auto la_ext =
          pose::look_at_extrinsics(frames[i].camera_position, pose::centroid(frames[i]), {0, 0, 1});
      angular_deg.push_back(
          cam ? pose::angular_error_degrees(la_ext.rotation, cam->extrinsics.rotation) : 0.0);

      uv = (opt.mode == "gt") ? gt : la;
      overlay_uv = gt;
    }

    const auto white_png = opt.out / "white" / (name + ".png");
    const auto overlay_png = opt.out / "overlay" / (name + ".png");
    pose::render_white(uv, white_png);
    pose::render_overlay(overlay_uv, opt.data / "frames" / (name + ".png"), overlay_png);
    pose::render_panel(overlay_png, white_png, opt.out / "panel" / (name + ".png"));
  }

  if (opt.mode != "lookat") {
    const auto joint_names = pose::load_joint_names(opt.data / "joint-names.txt");
    pose::write_coordinate_table(opt.out / "coords" / "all-2d-coordinates.csv",
                                 opt.out / "coords" / "all-2d-coordinates.tex", lookat_uv, gt_uv,
                                 joint_names, camera_ids);
    pose::write_error_tables(opt.out / "analysis", lookat_uv, gt_uv, angular_deg, camera_ids);
    std::cout << "wrote " << frames.size()
              << " white renders, coordinate table and error analysis to " << opt.out << "\n";
  } else {
    std::cout << "wrote " << frames.size() << " white renders to " << (opt.out / "white") << "\n";
  }
  return 0;
} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << "\n";
  return 1;
}
