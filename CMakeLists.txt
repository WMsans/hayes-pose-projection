cmake_minimum_required(VERSION 3.20)
project(hayes_pose_projection LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra)

include(cmake/Dependencies.cmake)

add_library(pose_core STATIC)
target_sources(pose_core PRIVATE src/pose_io.cpp src/skeleton.cpp src/camera.cpp src/analysis.cpp src/draw.cpp src/render.cpp src/tables.cpp)
target_include_directories(pose_core PUBLIC src ${raygui_SOURCE_DIR}/src)
target_link_libraries(pose_core PUBLIC glm::glm nlohmann_json::nlohmann_json raylib)

enable_testing()
add_executable(pose_tests tests/main_tests.cpp tests/test_pose_io.cpp tests/test_skeleton.cpp tests/test_camera.cpp tests/test_analysis.cpp tests/test_render.cpp tests/test_tables.cpp)
target_link_libraries(pose_tests PRIVATE pose_core doctest::doctest)
add_test(NAME unit COMMAND pose_tests)
set_tests_properties(unit PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(pose-project src/main_project.cpp)
target_link_libraries(pose-project PRIVATE pose_core)

add_executable(pose-explorer src/main_explorer.cpp)
target_link_libraries(pose-explorer PRIVATE pose_core)

add_test(NAME pose_project_rejects_invalid_mode
  COMMAND ${CMAKE_COMMAND}
    -DPOSE_PROJECT=$<TARGET_FILE:pose-project>
    -DCASE=invalid-mode
    -P ${CMAKE_SOURCE_DIR}/tests/check_pose_project_cli.cmake)

add_test(NAME pose_project_reports_input_error
  COMMAND ${CMAKE_COMMAND}
    -DPOSE_PROJECT=$<TARGET_FILE:pose-project>
    -DCASE=missing-input
    -P ${CMAKE_SOURCE_DIR}/tests/check_pose_project_cli.cmake)

add_test(NAME pose_project_supports_both_mode
  COMMAND ${CMAKE_COMMAND}
    -DPOSE_PROJECT=$<TARGET_FILE:pose-project>
    -DPOSE_DATA=${CMAKE_SOURCE_DIR}/data/Pose
    -DPOSE_OUTPUT=${CMAKE_BINARY_DIR}/both-task-8-output
    -DCASE=both
    -P ${CMAKE_SOURCE_DIR}/tests/check_pose_project_cli.cmake)
set_tests_properties(pose_project_supports_both_mode PROPERTIES
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

find_program(PDFLATEX_EXECUTABLE pdflatex)
if(PDFLATEX_EXECUTABLE)
  add_custom_target(report
    COMMAND ${PDFLATEX_EXECUTABLE} -interaction=nonstopmode report.tex
    COMMAND ${PDFLATEX_EXECUTABLE} -interaction=nonstopmode report.tex
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/report
    COMMENT "Building the write-up PDF")
endif()
