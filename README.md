# Voxel Frontier

Voxel Frontier is being built as a custom, performance-first voxel game with a C++ core and a browser build.

## Current browser build

- 3D voxel terrain generated from a world seed
- First-person mouse look and WASD movement
- Sprint and crouch
- Jump and collision
- Seven placeable block types
- Hold-to-mine blocks
- Right-click block placement
- Hotbar selection
- Day/night cycle
- Health/food HUD foundation
- Local world save/load
- Three.js browser renderer

## Native C++ core

The repository now contains a C++20 voxel core designed to become the main engine:

- Fixed-size 16x16x96 chunk storage using compact block IDs
- Deterministic seed-based terrain generation
- Visible-face culling during mesh generation
- CPU mesh output using indexed triangles
- CMake Release configuration with optimization flags
- Native smoke-test executable
- GitHub Actions build validation

### Build locally

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
./build/voxel_frontier 123456789
```

On Windows, run the executable from `build/` after configuring with your installed CMake generator.

## Architecture roadmap

1. C++ chunk/world engine
2. Greedy meshing and chunk streaming
3. Native renderer and player controller
4. Inventory, items and crafting
5. Caves, ores, water and biome generation
6. Lighting and world simulation
7. Mobs, AI, combat and survival
8. Structures and progression
9. Multiplayer/network layer
10. Emscripten/WebAssembly target using the same C++ core for the browser

The existing browser version remains available as an easy prototype while the C++ engine grows into the long-term game foundation.
