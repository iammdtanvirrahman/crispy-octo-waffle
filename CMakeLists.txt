cmake_minimum_required(VERSION 3.20)
project(voxel_frontier LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(voxel_core
    src/voxel_engine.cpp
    src/chunk_world.cpp
)
target_include_directories(voxel_core PUBLIC src)
target_compile_features(voxel_core PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(voxel_core PRIVATE /O2 /W4)
else()
  target_compile_options(voxel_core PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()

add_executable(voxel_frontier src/main.cpp)
target_link_libraries(voxel_frontier PRIVATE voxel_core)

option(VOXEL_BUILD_VIEWER "Build the native OpenGL viewer" OFF)
if(VOXEL_BUILD_VIEWER)
  include(FetchContent)
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
  )
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glfw)

  add_library(voxel_renderer src/native_renderer.cpp)
  target_include_directories(voxel_renderer PUBLIC src)
  target_compile_definitions(voxel_renderer PRIVATE VOXEL_NATIVE_GLFW=1)
  target_link_libraries(voxel_renderer PRIVATE glfw)
  target_compile_features(voxel_renderer PUBLIC cxx_std_20)

  add_executable(voxel_viewer src/native_viewer.cpp)
  target_compile_definitions(voxel_viewer PRIVATE VOXEL_NATIVE_GLFW=1)
  target_link_libraries(voxel_viewer PRIVATE voxel_core voxel_renderer)
endif()

# The same voxel_core library is intentionally suitable for a future Emscripten/WebAssembly build.
