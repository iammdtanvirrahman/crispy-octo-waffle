#include "camera.hpp"
#include "native_renderer.hpp"
#include "chunk_world.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

void updateSpectator(voxel::Camera& camera, const voxel::NativeRenderer& renderer, float dt) {
    const float speed = renderer.keyDown(340) ? 42.0f : 14.0f; // Left Shift = fast fly
    const float yaw = camera.yaw;
    const float pitch = camera.pitch;

    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);

    const voxel::Vec3 forward{sy * cp, sp, -cy * cp};
    const voxel::Vec3 right{cy, 0.0f, sy};

    voxel::Vec3 velocity{};
    if (renderer.keyDown(87)) { velocity.x += forward.x; velocity.y += forward.y; velocity.z += forward.z; }
    if (renderer.keyDown(83)) { velocity.x -= forward.x; velocity.y -= forward.y; velocity.z -= forward.z; }
    if (renderer.keyDown(68)) { velocity.x += right.x; velocity.y += right.y; velocity.z += right.z; }
    if (renderer.keyDown(65)) { velocity.x -= right.x; velocity.y -= right.y; velocity.z -= right.z; }
    if (renderer.keyDown(32)) velocity.y += 1.0f;  // Space = up
    if (renderer.keyDown(341)) velocity.y -= 1.0f; // Left Ctrl = down

    const float length = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
    if (length > 0.0f) {
        const float scale = speed * dt / length;
        camera.position.x += velocity.x * scale;
        camera.position.y += velocity.y * scale;
        camera.position.z += velocity.z * scale;
    }

    camera.mouseLook(renderer.mouseDX(), renderer.mouseDY());
}

} // namespace

int main(int argc, char** argv) {
    std::uint64_t seed = 123456789ULL;
    int radius = 6;
    if (argc > 1) {
        try { seed = std::stoull(argv[1]); }
        catch (...) { std::cerr << "Invalid seed; using default.\n"; }
    }
    if (argc > 2) {
        try { radius = std::max(1, std::stoi(argv[2])); }
        catch (...) { std::cerr << "Invalid radius; using 6.\n"; }
    }

    voxel::ChunkWorld world(seed);
    world.streamAround(0, 0, radius);
    world.rebuildMeshes();

    voxel::Mesh combined;
    for (const auto& [coord, chunk] : world.chunks()) {
        const voxel::Mesh* mesh = world.meshFor(coord.x, coord.z);
        if (!mesh) continue;
        const std::uint32_t base = static_cast<std::uint32_t>(combined.vertices.size());
        combined.vertices.insert(combined.vertices.end(), mesh->vertices.begin(), mesh->vertices.end());
        combined.indices.reserve(combined.indices.size() + mesh->indices.size());
        for (const std::uint32_t index : mesh->indices) combined.indices.push_back(base + index);
    }

    voxel::NativeRenderer renderer(1280, 720, "Voxel Frontier - World Generation Spectator");
    if (!renderer.valid()) {
        std::cerr << "Spectator viewer unavailable. Build with -DVOXEL_BUILD_VIEWER=ON.\n";
        return 2;
    }

    voxel::Camera camera;
    camera.position = {8.0f, 42.0f, 8.0f};
    camera.farPlane = 500.0f;
    renderer.uploadMesh(combined);

    std::cout << "World generation spectator\n"
              << "seed=" << seed << "\n"
              << "chunk_radius=" << radius << "\n"
              << "loaded_chunks=" << world.loadedChunkCount() << "\n"
              << "vertices=" << combined.vertices.size() << "\n"
              << "indices=" << combined.indices.size() << "\n"
              << "Controls: WASD move, Space up, Left Ctrl down, Left Shift fast, mouse look.\n";

    using clock = std::chrono::steady_clock;
    auto last = clock::now();
    while (!renderer.shouldClose()) {
        const auto now = clock::now();
        const float dt = std::min(0.05f, std::chrono::duration<float>(now - last).count());
        last = now;

        renderer.pollEvents();
        updateSpectator(camera, renderer, dt);
        renderer.clearMouseDelta();

        renderer.beginFrame();
        renderer.draw(camera, 1280.0f / 720.0f);
        renderer.endFrame();
    }

    return 0;
}
