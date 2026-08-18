# Voxel Frontier

A browser-based voxel survival game foundation, built for gradual expansion toward a full Minecraft-scale custom game.

## Current browser build

- 3D voxel terrain generated from a world seed
- First-person mouse look and WASD movement
- Sprint and crouch
- Jump and basic collision
- Seven placeable block types
- Hold-to-mine blocks
- Right-click block placement
- Hotbar selection
- Day/night cycle
- Health/food HUD foundation
- Local world save/load with automatic saving
- F3 HUD toggle and P manual save
- Performance-conscious Three.js renderer

## Planned architecture

The project is intentionally being grown in layers rather than as a one-off demo:

1. Core voxel engine and chunk meshing
2. Inventory, items and crafting
3. Procedural caves, ores, water and biomes
4. Lighting and better world streaming
5. Mobs, AI, combat and survival systems
6. Structures, villages and progression
7. Multiplayer/network layer
8. Optional Java/LWJGL desktop engine using the same gameplay concepts

The browser build remains the easy-to-run version on GitHub Pages while the engine can later gain a native Java desktop implementation.
