#include "player_controller.hpp"

#include <algorithm>
#include <cmath>

namespace voxel {

namespace {
constexpr int KEY_W = 87;
constexpr int KEY_A = 65;
constexpr int KEY_S = 83;
constexpr int KEY_D = 68;
constexpr int KEY_SPACE = 32;
constexpr int KEY_SHIFT = 340;
constexpr float WALK_SPEED = 6.0f;
constexpr float SPRINT_SPEED = 9.0f;
constexpr float JUMP_SPEED = 8.0f;
constexpr float GRAVITY = 22.0f;
constexpr float FLOOR_Y = 36.0f;
}

void PlayerController::update(float dt, const InputState& input, float mouseDX, float mouseDY, Camera& camera) noexcept {
    camera.mouseLook(mouseDX, mouseDY);

    float fx = -std::sin(camera.yaw);
    float fz = -std::cos(camera.yaw);
    float rx = std::cos(camera.yaw);
    float rz = -std::sin(camera.yaw);

    float x = 0.0f;
    float z = 0.0f;
    if (input.down(KEY_W)) { x += fx; z += fz; }
    if (input.down(KEY_S)) { x -= fx; z -= fz; }
    if (input.down(KEY_D)) { x += rx; z += rz; }
    if (input.down(KEY_A)) { x -= rx; z -= rz; }

    const float len = std::sqrt(x*x + z*z);
    if (len > 0.0001f) { x /= len; z /= len; }

    const float speed = input.down(KEY_SHIFT) ? SPRINT_SPEED : WALK_SPEED;
    camera.position.x += x * speed * dt;
    camera.position.z += z * speed * dt;

    if (input.down(KEY_SPACE) && grounded_) {
        verticalVelocity_ = JUMP_SPEED;
        grounded_ = false;
    }

    verticalVelocity_ -= GRAVITY * dt;
    camera.position.y += verticalVelocity_ * dt;

    if (camera.position.y <= FLOOR_Y) {
        camera.position.y = FLOOR_Y;
        verticalVelocity_ = 0.0f;
        grounded_ = true;
    }
}

} // namespace voxel
