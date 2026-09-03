#include <cstdio>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#include "analysis.hpp"
#include "render.hpp"

namespace {

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
  const auto cameras = pose::load_calibration("third_party/h36m/camera-parameters.json", "S1");

  pose::begin_offscreen();
  for (std::size_t i = 0; i < frames.size(); ++i) {
    const auto mode = (opt.mode == "gt") ? pose::Mode::Gt : pose::Mode::LookAt;
    const auto uv = pose::project_frame(frames[i], mode, cameras, focal);
    pose::render_white(uv, opt.out / "white" / (frame_name(i) + ".png"));
  }
  pose::end_offscreen();

  std::cout << "wrote " << frames.size() << " white renders to " << (opt.out / "white") << "\n";
  return 0;
} catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << "\n";
  return 1;
}
