#include "tables.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>

#include "analysis.hpp"
#include "pose_io.hpp"

namespace pose {
namespace {

ProjectedFrame complete_projection(const std::vector<glm::dvec2>& uv) {
  if (uv.size() != kJointCount)
    throw std::runtime_error("write tables: joint count mismatch");
  ProjectedFrame result;
  result.uv = uv;
  return result;
}

void validate_frame_counts(const std::vector<ProjectedFrame>& lookat,
                           const std::vector<ProjectedFrame>& gt,
                           const std::vector<std::string>& camera_ids) {
  if (lookat.size() != gt.size() || lookat.size() != camera_ids.size())
    throw std::runtime_error("write tables: frame count mismatch");
  for (const auto& projection : lookat)
    if (projection.uv.size() != kJointCount)
      throw std::runtime_error("write tables: joint count mismatch");
  for (const auto& projection : gt)
    if (projection.uv.size() != kJointCount)
      throw std::runtime_error("write tables: joint count mismatch");
}

std::string coordinate_value(const ProjectedFrame& projection, std::size_t joint, bool x) {
  if (projection.joint_status[joint] != JointProjectionStatus::Valid) return {};
  const double value = x ? projection.uv[joint].x : projection.uv[joint].y;
  return std::isfinite(value) ? fixed(value, 2) : std::string{};
}

std::string projection_status(const ProjectedFrame& projection) {
  bool unmatched = !projection.camera_matched;
  bool behind = false;
  for (const auto status : projection.joint_status) {
    unmatched = unmatched || status == JointProjectionStatus::UnmatchedCamera;
    behind = behind || status == JointProjectionStatus::BehindCamera;
  }
  if (unmatched) return "unmatched-camera";
  if (behind) return "behind-camera";
  return "valid";
}

}  // namespace

void write_coordinate_table(const std::filesystem::path& csv, const std::filesystem::path& tex,
                            const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                            const std::vector<std::vector<glm::dvec2>>& gt_uv,
                            const std::vector<std::string>& joint_names,
                            const std::vector<std::string>& camera_ids) {
  std::vector<ProjectedFrame> lookat;
  std::vector<ProjectedFrame> gt;
  lookat.reserve(lookat_uv.size());
  gt.reserve(gt_uv.size());
  for (const auto& uv : lookat_uv) lookat.push_back(complete_projection(uv));
  for (const auto& uv : gt_uv) gt.push_back(complete_projection(uv));
  write_coordinate_table(csv, tex, lookat, gt, joint_names, camera_ids);
}

void write_coordinate_table(const std::filesystem::path& csv, const std::filesystem::path& tex,
                            const std::vector<ProjectedFrame>& lookat,
                            const std::vector<ProjectedFrame>& gt,
                            const std::vector<std::string>& joint_names,
                            const std::vector<std::string>& camera_ids) {
  validate_frame_counts(lookat, gt, camera_ids);
  if (joint_names.size() != kJointCount)
    throw std::runtime_error("write_coordinate_table: joint count mismatch");

  std::vector<std::vector<std::string>> rows;
  rows.reserve(lookat.size() * kJointCount);
  for (std::size_t f = 0; f < lookat.size(); ++f) {
    for (int j = 0; j < kJointCount; ++j) {
      rows.push_back({std::to_string(f), camera_ids[f], std::to_string(j), joint_names[j],
                      coordinate_value(lookat[f], j, true), coordinate_value(lookat[f], j, false),
                      coordinate_value(gt[f], j, true), coordinate_value(gt[f], j, false)});
    }
  }
  write_csv(csv, {"frame", "camera", "joint", "name", "u_lookat", "v_lookat", "u_gt", "v_gt"},
            rows);

  if (!tex.parent_path().empty()) std::filesystem::create_directories(tex.parent_path());
  std::ofstream out(tex);
  if (!out) throw std::runtime_error("cannot write " + tex.string());
  out << "\\begin{longtable}{rlrlrrrr}\n\\toprule\n"
      << "Frame & Camera & \\# & Joint & $u_{\\text{look-at}}$ & $v_{\\text{look-at}}$ & "
         "$u_{\\text{gt}}$ & $v_{\\text{gt}}$ \\\\\n\\midrule\n\\endhead\n";
  for (const auto& row : rows) {
    for (std::size_t i = 0; i < row.size(); ++i) out << (i ? " & " : "") << row[i];
    out << " \\\\\n";
  }
  out << "\\bottomrule\n\\end{longtable}\n";
}

void write_error_tables(const std::filesystem::path& out_dir,
                        const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                        const std::vector<std::vector<glm::dvec2>>& gt_uv,
                        const std::vector<double>& angular_deg,
                        const std::vector<std::string>& camera_ids) {
  std::vector<ProjectedFrame> lookat;
  std::vector<ProjectedFrame> gt;
  lookat.reserve(lookat_uv.size());
  gt.reserve(gt_uv.size());
  for (const auto& uv : lookat_uv) {
    if (uv.size() != kJointCount)
      throw std::runtime_error("write_error_tables: joint count mismatch");
    lookat.push_back(complete_projection(uv));
  }
  for (const auto& uv : gt_uv) {
    if (uv.size() != kJointCount)
      throw std::runtime_error("write_error_tables: joint count mismatch");
    gt.push_back(complete_projection(uv));
  }
  write_error_tables(out_dir, lookat, gt, angular_deg, camera_ids);
}

void write_error_tables(const std::filesystem::path& out_dir,
                        const std::vector<ProjectedFrame>& lookat,
                        const std::vector<ProjectedFrame>& gt,
                        const std::vector<double>& angular_deg,
                        const std::vector<std::string>& camera_ids) {
  validate_frame_counts(lookat, gt, camera_ids);
  if (lookat.size() != angular_deg.size())
    throw std::runtime_error("write_error_tables: frame count mismatch");

  std::vector<std::vector<std::string>> rows;
  std::vector<std::vector<std::string>> angular_rows;
  for (std::size_t f = 0; f < lookat.size(); ++f) {
    if (!lookat[f].complete() || !gt[f].complete()) continue;
    const auto e = joint_error(lookat[f].uv, gt[f].uv);
    rows.push_back({std::to_string(f), camera_ids[f], fixed(e.mean_px, 2), fixed(e.max_px, 2),
                    std::to_string(e.worst_joint), fixed(angular_deg[f], 4)});
    angular_rows.push_back({std::to_string(f), camera_ids[f], fixed(angular_deg[f], 4)});
  }
  write_csv(out_dir / "joint-error.csv",
            {"frame", "camera", "mean_px", "max_px", "worst_joint", "angular_deg"}, rows);
  write_csv(out_dir / "camera-angular-error.csv", {"frame", "camera", "angular_deg"},
            angular_rows);
}

void write_projection_status(const std::filesystem::path& path,
                             const std::vector<ProjectedFrame>& lookat,
                             const std::vector<ProjectedFrame>& gt,
                             const std::vector<std::string>& camera_ids) {
  validate_frame_counts(lookat, gt, camera_ids);
  std::vector<std::vector<std::string>> rows;
  rows.reserve(lookat.size());
  for (std::size_t f = 0; f < lookat.size(); ++f) {
    rows.push_back({std::to_string(f), camera_ids[f], projection_status(lookat[f]),
                    std::to_string(lookat[f].invalid_joint_count()), projection_status(gt[f]),
                    std::to_string(gt[f].invalid_joint_count())});
  }
  write_csv(path,
            {"frame", "camera", "lookat_status", "lookat_invalid_joints", "gt_status",
             "gt_invalid_joints"},
            rows);
}

}  // namespace pose
