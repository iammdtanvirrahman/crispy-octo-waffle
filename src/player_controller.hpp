#pragma once

#include "camera.hpp"

#include <unordered_map>

namespace voxel {

class InputState {
public:
    void setKey(int key, bool down) noexcept { keys_[key] = down; }
    [[nodiscard]] bool down(int key) const noexcept {
        const auto it = keys_.find(key);
        return it != keys_.end() && it->second;
    }
private:
    std::unordered_map<int, bool> keys_;
};

class PlayerController {
public:
    void update(float dt, const InputState& input, float mouseDX, float mouseDY, Camera& camera) noexcept;
private:
    float verticalVelocity_ = 0.0f;
    bool grounded_ = false;
};

} // namespace voxel
