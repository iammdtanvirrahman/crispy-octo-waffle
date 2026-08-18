cmake_minimum_required(VERSION 3.20)
project(voxel_frontier LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(voxel_core
    src/voxel_engine.cpp
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

# Browser target is planned as a separate Emscripten build so the same
# voxel_core library can later be compiled to WebAssembly.
