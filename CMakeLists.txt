cmake_minimum_required(VERSION 3.24)
project(Helios3DEngine VERSION 0.1.0 LANGUAGES CXX)

# CMake 4 removes compatibility with very old policy versions used by some
# fetched third-party dependencies. Keep policy minimum explicit so configure
# succeeds without extra CLI flags.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HELIOS_BUILD_TESTS "Build tests" OFF)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v0.1.36
)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.8-docking
)

FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_TAG master
)

# Configure glad before it is populated (newer glad CMake layout).
set(GLAD_API "gl=4.1" CACHE STRING "" FORCE)
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_REPRODUCIBLE ON CACHE BOOL "" FORCE)
set(PYTHON_EXECUTABLE "C:/Users/user/AppData/Local/Programs/Python/Python313/python.exe" CACHE FILEPATH "" FORCE)

FetchContent_MakeAvailable(glfw glad glm nlohmann_json imgui imguizmo)

set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui_lib PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui_lib PUBLIC glfw)

add_library(imguizmo_lib STATIC
  ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)
target_include_directories(imguizmo_lib PUBLIC
  ${imguizmo_SOURCE_DIR}
  ${imgui_SOURCE_DIR}
)
target_link_libraries(imguizmo_lib PUBLIC imgui_lib)
target_compile_definitions(imguizmo_lib PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

add_executable(Helios3DEngine
  src/main.cpp
  src/core/Application.cpp
  src/core/Window.cpp
  src/core/Log.cpp
  src/core/Time.cpp
  src/renderer/Renderer.cpp
  src/renderer/Camera.cpp
  src/scene/Scene.cpp
  src/scene/SceneSerializer.cpp
  src/editor/EditorLayer.cpp
)

target_include_directories(Helios3DEngine PRIVATE
  src
  ${glm_SOURCE_DIR}
  ${nlohmann_json_SOURCE_DIR}/include
)

target_link_libraries(Helios3DEngine PRIVATE
  glfw
  glad
  imgui_lib
  imguizmo_lib
  nlohmann_json::nlohmann_json
)

if (WIN32)
  target_link_libraries(Helios3DEngine PRIVATE opengl32)
endif()

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
