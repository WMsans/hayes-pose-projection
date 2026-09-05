#include "analysis.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <glm/geometric.hpp>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

#include "skeleton.hpp"

namespace pose {

FrameError joint_error(const std::vector<glm::dvec2>& a, const std::vector<glm::dvec2>& b) {
  if (a.size() != b.size() || a.empty()) throw std::runtime_error("joint_error: size mismatch");
  FrameError e;
  double sum = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const double d = glm::length(a[i] - b[i]);
    sum += d;
    if (d > e.max_px) {
      e.max_px = d;
      e.worst_joint = static_cast<int>(i);
    }
  }
  e.mean_px = sum / static_cast<double>(a.size());
  return e;
}

double angular_error_degrees(const glm::dmat3& a, const glm::dmat3& b) {
  const glm::dmat3 d = glm::transpose(a) * b;
  const double trace = d[0][0] + d[1][1] + d[2][2];
  const double cos_theta = std::clamp((trace - 1.0) / 2.0, -1.0, 1.0);
  constexpr double pi = 3.141592653589793238462643383279502884;
  return std::acos(cos_theta) * 180.0 / pi;
}

std::array<double, 13> limb_lengths(const Frame& frame) {
  std::array<double, 13> lengths{};
  const auto& bs = bones();
  for (std::size_t i = 0; i < bs.size(); ++i)
    lengths[i] = glm::length(frame.joints[bs[i].b] - frame.joints[bs[i].a]);
  return lengths;
}

std::string fixed(double value, int decimals) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::fixed << std::setprecision(decimals) << value;
  return out.str();
}

void write_csv(const std::filesystem::path& path, const std::vector<std::string>& header,
               const std::vector<std::vector<std::string>>& rows) {
  if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) throw std::runtime_error("cannot write " + path.string());
  for (std::size_t i = 0; i < header.size(); ++i) out << (i ? "," : "") << header[i];
  out << "\n";
  for (const auto& row : rows) {
    if (row.size() != header.size()) throw std::runtime_error("write_csv: column count mismatch");
    for (std::size_t i = 0; i < row.size(); ++i) out << (i ? "," : "") << row[i];
    out << "\n";
  }
}

}  // namespace pose
