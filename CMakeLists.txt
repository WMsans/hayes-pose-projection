cmake_minimum_required(VERSION 3.20)
project(hayes_pose_projection LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra)

include(cmake/Dependencies.cmake)

add_library(pose_core STATIC)
target_sources(pose_core PRIVATE src/placeholder.cpp)
target_include_directories(pose_core PUBLIC src ${raygui_SOURCE_DIR}/src)
target_link_libraries(pose_core PUBLIC glm::glm nlohmann_json::nlohmann_json raylib)

enable_testing()
add_executable(pose_tests tests/test_smoke.cpp)
target_link_libraries(pose_tests PRIVATE pose_core doctest::doctest)
add_test(NAME unit COMMAND pose_tests)
