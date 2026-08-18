#include "camera.hpp"
#include "native_renderer.hpp"
#include "player_controller.hpp"
#include "voxel_engine.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>

int main(int argc, char** argv) {
    std::uint64_t seed = 123456789ULL;
    if (argc > 1) {
        try { seed = std::stoull(argv[1]); }
        catch (...) { std::cerr << "Invalid seed; using default.\n"; }
    }

    voxel::Chunk chunk(0, 0);
    voxel::WorldGenerator generator(seed);
    generator.generate(chunk);
    const voxel::Mesh mesh = chunk.buildVisibleMesh();

    voxel::NativeRenderer renderer(1280, 720, "Voxel Frontier - C++ Engine");
    if (!renderer.valid()) {
        std::cerr << "Native viewer unavailable. Build with -DVOXEL_BUILD_VIEWER=ON.\n";
        return 2;
    }

    voxel::Camera camera;
    camera.position = {8.0f, 38.0f, 10.0f};
    voxel::PlayerController controller;
    voxel::InputState input;
    renderer.uploadMesh(mesh);

    using clock = std::chrono::steady_clock;
    auto last = clock::now();
    while (!renderer.shouldClose()) {
        const auto now = clock::now();
        const float dt = std::min(0.05f, std::chrono::duration<float>(now - last).count());
        last = now;

        renderer.pollEvents();
        input.setKey(87, renderer.keyDown(87));   // W
        input.setKey(65, renderer.keyDown(65));   // A
        input.setKey(83, renderer.keyDown(83));   // S
        input.setKey(68, renderer.keyDown(68));   // D
        input.setKey(32, renderer.keyDown(32));   // Space
        input.setKey(340, renderer.keyDown(340)); // Left Shift
        controller.update(dt, input, renderer.mouseDX(), renderer.mouseDY(), camera);
        renderer.clearMouseDelta();

        renderer.beginFrame();
        renderer.draw(camera, 1280.0f / 720.0f);
        renderer.endFrame();
    }

    return 0;
}
