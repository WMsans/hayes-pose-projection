#include "tables.hpp"

#include <fstream>
#include <stdexcept>

#include "analysis.hpp"
#include "pose_io.hpp"

namespace pose {

void write_coordinate_table(const std::filesystem::path& csv, const std::filesystem::path& tex,
                            const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                            const std::vector<std::vector<glm::dvec2>>& gt_uv,
                            const std::vector<std::string>& joint_names,
                            const std::vector<std::string>& camera_ids) {
  if (lookat_uv.size() != gt_uv.size() || lookat_uv.size() != camera_ids.size())
    throw std::runtime_error("write_coordinate_table: frame count mismatch");
  if (joint_names.size() != kJointCount)
    throw std::runtime_error("write_coordinate_table: joint count mismatch");

  std::vector<std::vector<std::string>> rows;
  rows.reserve(lookat_uv.size() * kJointCount);
  for (std::size_t f = 0; f < lookat_uv.size(); ++f) {
    if (lookat_uv[f].size() != kJointCount || gt_uv[f].size() != kJointCount)
      throw std::runtime_error("write_coordinate_table: joint count mismatch");
    for (int j = 0; j < kJointCount; ++j) {
      rows.push_back({std::to_string(f), camera_ids[f], std::to_string(j), joint_names[j],
                      fixed(lookat_uv[f][j].x, 2), fixed(lookat_uv[f][j].y, 2),
                      fixed(gt_uv[f][j].x, 2), fixed(gt_uv[f][j].y, 2)});
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
  if (lookat_uv.size() != gt_uv.size() || lookat_uv.size() != angular_deg.size() ||
      lookat_uv.size() != camera_ids.size())
    throw std::runtime_error("write_error_tables: frame count mismatch");

  std::vector<std::vector<std::string>> rows;
  rows.reserve(lookat_uv.size());
  std::vector<std::vector<std::string>> angular_rows;
  angular_rows.reserve(lookat_uv.size());
  for (std::size_t f = 0; f < lookat_uv.size(); ++f) {
    if (lookat_uv[f].size() != kJointCount || gt_uv[f].size() != kJointCount)
      throw std::runtime_error("write_error_tables: joint count mismatch");
    const auto e = joint_error(lookat_uv[f], gt_uv[f]);
    rows.push_back({std::to_string(f), camera_ids[f], fixed(e.mean_px, 2), fixed(e.max_px, 2),
                    std::to_string(e.worst_joint), fixed(angular_deg[f], 4)});
    angular_rows.push_back(
        {std::to_string(f), camera_ids[f], fixed(angular_deg[f], 4)});
  }
  write_csv(out_dir / "joint-error.csv",
            {"frame", "camera", "mean_px", "max_px", "worst_joint", "angular_deg"}, rows);
  write_csv(out_dir / "camera-angular-error.csv", {"frame", "camera", "angular_deg"},
            angular_rows);
}

}  // namespace pose
