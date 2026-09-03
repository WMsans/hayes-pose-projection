#include "camera.hpp"

#include <fstream>
#include <glm/geometric.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace pose {

glm::dmat3 look_at_rotation(const glm::dvec3& eye, const glm::dvec3& target,
                            const glm::dvec3& world_up) {
  const glm::dvec3 forward = glm::normalize(target - eye);
  glm::dvec3 right = glm::cross(forward, world_up);
  const double len = glm::length(right);
  if (len < 1e-9)
    throw std::runtime_error("look_at: view direction is parallel to the world-up axis");
  right /= len;
  const glm::dvec3 up = glm::cross(right, forward);

  // Rows are the camera basis; image y points down, hence -up.
  glm::dmat3 R(1.0);
  for (int c = 0; c < 3; ++c) {
    R[c][0] = right[c];
    R[c][1] = -up[c];
    R[c][2] = forward[c];
  }
  return R;
}

Extrinsics look_at_extrinsics(const glm::dvec3& eye, const glm::dvec3& target,
                              const glm::dvec3& world_up) {
  Extrinsics e;
  e.rotation = look_at_rotation(eye, target, world_up);
  e.translation = -(e.rotation * eye);
  return e;
}

glm::dvec3 centroid(const Frame& frame) {
  glm::dvec3 sum(0.0);
  for (const auto& j : frame.joints) sum += j;
  return sum / static_cast<double>(kJointCount);
}

glm::dvec2 project(const glm::dvec3& world, const Extrinsics& ext, const Intrinsics& k,
                   bool apply_distortion) {
  const glm::dvec3 c = ext.rotation * world + ext.translation;
  if (c.z <= 0.0) throw std::runtime_error("point is at or behind the image plane");

  double x = c.x / c.z;
  double y = c.y / c.z;

  if (apply_distortion && k.has_distortion) {
    const double k1 = k.distortion[0], k2 = k.distortion[1];
    const double p1 = k.distortion[2], p2 = k.distortion[3], k3 = k.distortion[4];
    const double r2 = x * x + y * y;
    const double radial = 1.0 + r2 * (k1 + r2 * (k2 + r2 * k3));
    const double xd = x * radial + 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
    const double yd = y * radial + p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
    x = xd;
    y = yd;
  }
  return {k.fx * x + k.cx, k.fy * y + k.cy};
}

Intrinsics challenge_intrinsics(double focal) {
  Intrinsics k;
  k.fx = focal;
  k.fy = focal;
  k.cx = 500.0;
  k.cy = 500.0;
  k.has_distortion = false;
  return k;
}

glm::dvec3 camera_center(const Extrinsics& ext) {
  return -(glm::transpose(ext.rotation) * ext.translation);
}

std::vector<CalibratedCamera> load_calibration(const std::filesystem::path& json_path,
                                               const std::string& subject) {
  std::ifstream in(json_path);
  if (!in) throw std::runtime_error("cannot open " + json_path.string());
  const nlohmann::json root = nlohmann::json::parse(in);
  if (!root.contains("extrinsics") || !root["extrinsics"].contains(subject))
    throw std::runtime_error("no extrinsics for subject " + subject);

  std::vector<CalibratedCamera> cameras;
  for (const auto& [id, ext_json] : root["extrinsics"][subject].items()) {
    CalibratedCamera cam;
    cam.id = id;

    const auto& K = root["intrinsics"][id]["calibration_matrix"];
    cam.intrinsics.fx = K[0][0].get<double>();
    cam.intrinsics.fy = K[1][1].get<double>();
    cam.intrinsics.cx = K[0][2].get<double>();
    cam.intrinsics.cy = K[1][2].get<double>();
    const auto& d = root["intrinsics"][id]["distortion"];
    for (int i = 0; i < 5; ++i) cam.intrinsics.distortion[i] = d[i].get<double>();
    cam.intrinsics.has_distortion = true;

    // R is row-major 3x3; glm::dmat3 is column-major, so index as R[col][row].
    const auto& R = ext_json["R"];
    for (int r = 0; r < 3; ++r)
      for (int c = 0; c < 3; ++c) cam.extrinsics.rotation[c][r] = R[r][c].get<double>();

    const auto& t = ext_json["t"];
    for (int r = 0; r < 3; ++r)
      cam.extrinsics.translation[r] = t[r].is_array() ? t[r][0].get<double>() : t[r].get<double>();

    cam.center = camera_center(cam.extrinsics);
    cameras.push_back(std::move(cam));
  }
  if (cameras.empty()) throw std::runtime_error("no cameras for subject " + subject);
  return cameras;
}

const CalibratedCamera* identify(const glm::dvec3& camera_position,
                                 const std::vector<CalibratedCamera>& cameras,
                                 double tolerance_mm) {
  const CalibratedCamera* best = nullptr;
  double best_distance = tolerance_mm;
  for (const auto& cam : cameras) {
    const double d = glm::length(cam.center - camera_position);
    if (d <= best_distance) {
      best_distance = d;
      best = &cam;
    }
  }
  return best;
}

}  // namespace pose
