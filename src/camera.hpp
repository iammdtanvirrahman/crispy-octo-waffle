#pragma once

#include <array>
#include <cmath>

namespace voxel {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Camera {
    Vec3 position{0.0f, 36.0f, 6.0f};
    float yaw = 3.14159265f;
    float pitch = 0.0f;
    float fov = 75.0f;
    float nearPlane = 0.05f;
    float farPlane = 180.0f;

    void mouseLook(float dx, float dy, float sensitivity = 0.0025f) noexcept {
        yaw -= dx * sensitivity;
        pitch -= dy * sensitivity;
        constexpr float limit = 1.55334306f;
        if (pitch > limit) pitch = limit;
        if (pitch < -limit) pitch = -limit;
    }
};

} // namespace voxel
