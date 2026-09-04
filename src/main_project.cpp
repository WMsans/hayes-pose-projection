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

  std::vector<pose::ProjectedFrame> lookat_results;
  std::vector<pose::ProjectedFrame> gt_results;
  std::vector<std::string> camera_ids;
  std::vector<double> angular_deg;

  OffscreenScope offscreen;
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const std::string name = frame_name(i);
    const auto lookat =
        pose::project_frame_status(frames[i], pose::Mode::LookAt, cameras, focal, false);
    if (lookat.invalid_joint_count() != 0)
      std::cerr << "warning: frame " << i << ": look-at has " << lookat.invalid_joint_count()
                << " behind-camera joints; invalid joints are omitted\n";

    pose::ProjectedFrame gt;
    const pose::CalibratedCamera* cam = nullptr;
    if (opt.mode != "lookat") {
      // Identify before projecting GT so an unknown position can never be guessed.
      cam = pose::identify(frames[i].camera_position, cameras, 1.0);
      if (cam == nullptr) {
        std::cerr << "warning: frame " << i
                  << ": no published camera matches this position; using look-at only\n";
      }
      gt = pose::project_frame_status(frames[i], pose::Mode::Gt, cameras, focal, true);
      if (gt.invalid_joint_count() != 0 && gt.camera_matched)
        std::cerr << "warning: frame " << i << ": ground-truth has "
                  << gt.invalid_joint_count()
                  << " behind-camera joints; GT comparison and overlay are skipped\n";

      lookat_results.push_back(lookat);
      gt_results.push_back(gt);
      camera_ids.push_back(cam != nullptr ? cam->id : "unknown");
      const auto la_ext = pose::look_at_extrinsics(frames[i].camera_position,
                                                    pose::centroid(frames[i]), {0, 0, 1});
      angular_deg.push_back(
          cam != nullptr ? pose::angular_error_degrees(la_ext.rotation, cam->extrinsics.rotation)
                         : 0.0);
    }

    const auto& primary = (opt.mode == "gt" && gt.complete()) ? gt : lookat;
    const auto white_png = opt.out / "white" / (name + ".png");
    const auto overlay_png = opt.out / "overlay" / (name + ".png");
    pose::render_white(primary.uv, white_png);
    if (opt.mode != "lookat" && gt.complete()) {
      pose::render_overlay(gt.uv, opt.data / "frames" / (name + ".png"), overlay_png);
    } else if (opt.mode != "lookat") {
      std::cerr << "warning: frame " << i << ": skipping GT overlay\n";
    }
    pose::render_panel(overlay_png, white_png, opt.out / "panel" / (name + ".png"));
  }

  if (opt.mode != "lookat") {
    const auto joint_names = pose::load_joint_names(opt.data / "joint-names.txt");
    pose::write_coordinate_table(opt.out / "coords" / "all-2d-coordinates.csv",
                                 opt.out / "coords" / "all-2d-coordinates.tex", lookat_results,
                                 gt_results, joint_names, camera_ids);
    pose::write_error_tables(opt.out / "analysis", lookat_results, gt_results, angular_deg,
                             camera_ids);
    pose::write_projection_status(opt.out / "analysis" / "projection-status.csv", lookat_results,
                                  gt_results, camera_ids);
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
