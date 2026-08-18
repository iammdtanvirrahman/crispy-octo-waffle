#pragma once

#include "camera.hpp"
#include "voxel_engine.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace voxel {

class NativeRenderer {
public:
    NativeRenderer(int width, int height, std::string title);
    ~NativeRenderer();

    NativeRenderer(const NativeRenderer&) = delete;
    NativeRenderer& operator=(const NativeRenderer&) = delete;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool shouldClose() const noexcept;

    void beginFrame() noexcept;
    void endFrame() noexcept;
    void uploadMesh(const Mesh& mesh);
    void draw(const Camera& camera, float aspect) noexcept;
    void pollEvents() noexcept;

    [[nodiscard]] float mouseDX() const noexcept { return mouseDX_; }
    [[nodiscard]] float mouseDY() const noexcept { return mouseDY_; }
    void clearMouseDelta() noexcept { mouseDX_ = mouseDY_ = 0.0f; }
    [[nodiscard]] bool keyDown(int key) const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    float mouseDX_ = 0.0f;
    float mouseDY_ = 0.0f;
};

} // namespace voxel
