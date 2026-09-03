include(FetchContent)

# Several pinned dependencies declare cmake_minimum_required below 3.5,
# which CMake 4.x rejects outright. This floor keeps them configurable.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # raylib
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)   # raylib
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)  # nlohmann

FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5 GIT_SHALLOW TRUE)
FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3 GIT_SHALLOW TRUE)
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11 GIT_SHALLOW TRUE)
# raygui has no CMakeLists.txt at its root, so MakeAvailable only populates it;
# we add ${raygui_SOURCE_DIR}/src to the include path by hand.
FetchContent_Declare(raygui
  GIT_REPOSITORY https://github.com/raysan5/raygui.git
  GIT_TAG 4.0 GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(raylib glm nlohmann_json doctest raygui)
