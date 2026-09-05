#pragma once

#include <filesystem>
#include <glm/vec2.hpp>
#include <string>
#include <vector>

#include "render.hpp"

namespace pose {

// The single table the challenge asks for: every 2D coordinate used, in both modes.
void write_coordinate_table(const std::filesystem::path& csv, const std::filesystem::path& tex,
                            const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                            const std::vector<std::vector<glm::dvec2>>& gt_uv,
                            const std::vector<std::string>& joint_names,
                            const std::vector<std::string>& camera_ids);
void write_coordinate_table(const std::filesystem::path& csv, const std::filesystem::path& tex,
                            const std::vector<ProjectedFrame>& lookat,
                            const std::vector<ProjectedFrame>& gt,
                            const std::vector<std::string>& joint_names,
                            const std::vector<std::string>& camera_ids);

void write_error_tables(const std::filesystem::path& out_dir,
                        const std::vector<std::vector<glm::dvec2>>& lookat_uv,
                        const std::vector<std::vector<glm::dvec2>>& gt_uv,
                        const std::vector<double>& angular_deg,
                        const std::vector<std::string>& camera_ids);
void write_error_tables(const std::filesystem::path& out_dir,
                        const std::vector<ProjectedFrame>& lookat,
                        const std::vector<ProjectedFrame>& gt,
                        const std::vector<double>& angular_deg,
                        const std::vector<std::string>& camera_ids);
void write_projection_status(const std::filesystem::path& path,
                             const std::vector<ProjectedFrame>& lookat,
                             const std::vector<ProjectedFrame>& gt,
                             const std::vector<std::string>& camera_ids);

}  // namespace pose
