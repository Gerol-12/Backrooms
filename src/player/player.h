#pragma once
#include "player/camera.h"
#include "world/world.h"

class Player {
public:
    Player();
    Camera& camera() { return cam_; }
    const Camera& camera() const { return cam_; }
    void update(const World& world, const bool* keys, float dt);
    enum class Mode { Walk, Run, Slow, Crouch };
    Mode mode() const { return mode_; }
    const char* modeName() const;
private:
    Camera cam_;
    Mode mode_ = Mode::Walk;
    bool isGrounded = false;
    float gravity = 24.0f;
    bool checkCollision(const World& world, float x, float y, float z, float height) const;
};