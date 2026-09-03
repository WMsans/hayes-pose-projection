#include "pose_io.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace pose {
namespace {

[[noreturn]] void fail(const std::filesystem::path& p, std::size_t line, const std::string& why) {
  throw std::runtime_error(p.string() + ":" + std::to_string(line) + ": " + why);
}

}  // namespace

std::vector<Frame> load_poses(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open " + path.string());

  std::vector<Frame> frames;
  std::string line;
  std::size_t line_no = 0;
  while (std::getline(in, line)) {
    ++line_no;
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

    std::istringstream ss(line);
    std::vector<double> values;
    double v = 0.0;
    while (ss >> v) values.push_back(v);
    if (!ss.eof()) fail(path, line_no, "non-numeric token in row");
    if (values.size() != static_cast<std::size_t>(kPoseColumns))
      fail(path, line_no, "expected " + std::to_string(kPoseColumns) + " columns, got " +
                              std::to_string(values.size()));
    for (double x : values)
      if (!std::isfinite(x)) fail(path, line_no, "non-finite value");

    Frame f;
    f.camera_position = {values[0], values[1], values[2]};
    for (int j = 0; j < kJointCount; ++j)
      f.joints[j] = {values[3 + 3 * j], values[4 + 3 * j], values[5 + 3 * j]};
    frames.push_back(f);
  }
  if (frames.empty()) throw std::runtime_error("no pose rows in " + path.string());
  return frames;
}

double load_focal(const std::filesystem::path& path) {
  std::ifstream in(path);
  double f = 0.0;
  if (!in || !(in >> f) || !std::isfinite(f) || f <= 0.0)
    throw std::runtime_error("bad focal length in " + path.string());
  return f;
}

std::vector<std::string> load_joint_names(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open " + path.string());
  std::vector<std::string> names;
  std::string line;
  while (std::getline(in, line)) {
    const auto a = line.find('\'');
    const auto b = line.rfind('\'');
    if (a == std::string::npos || b <= a) continue;
    names.push_back(line.substr(a + 1, b - a - 1));
  }
  if (names.empty()) throw std::runtime_error("no joint names in " + path.string());
  return names;
}

}  // namespace pose
