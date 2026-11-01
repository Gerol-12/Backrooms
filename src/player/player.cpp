// src/player/player.cpp

#include "player/player.h"
#include "core/math_utils.h"

#include <SDL3/SDL.h>
#include <cmath>

Player::Player() = default;

const char* Player::modeName() const {
    switch (mode_) {
        case Mode::Walk:   return "WALK";
        case Mode::Run:    return "RUN";
        case Mode::Slow:   return "SLOW";
        case Mode::Crouch: return "CROUCH";
    }
    return "?";
}

bool Player::tryMove(const World& world, float dx, float dz) {
    if (dx == 0.0f && dz == 0.0f) return false;

    auto isValid = [&](float x, float z) -> bool {
        if (!world.isFloor((int)std::floor(x), (int)std::floor(z))) return false;
        for (int i = 0; i < 8; ++i) {
            float angle = i * (3.14159265f / 4.0f);
            float px = x + std::cos(angle) * PLAYER_RADIUS;
            float pz = z + std::sin(angle) * PLAYER_RADIUS;
            if (!world.isFloor((int)std::floor(px), (int)std::floor(pz))) return false;
        }
        return true;
    };

    glm::vec3& pos = cam_.position();

    if (!isValid(pos.x, pos.z)) {
        for (int i = 0; i < 8; ++i) {
            float angle = i * (3.14159265f / 4.0f);
            float nx = pos.x + std::cos(angle) * 1.0f;
            float nz = pos.z + std::sin(angle) * 1.0f;
            if (isValid(nx, nz)) {
                pos.x = nx;
                pos.z = nz;
                return true;
            }
        }
        return false;
    }

    float oldX = pos.x;
    float oldZ = pos.z;

    if (isValid(oldX + dx, oldZ + dz)) {
        pos.x += dx;
        pos.z += dz;
        return true;
    }

    bool moved = false;
    if (isValid(oldX + dx, oldZ)) {
        pos.x += dx;
        moved = true;
    } else {
        cam_.velocity().x = 0.0f;
    }

    if (isValid(oldX, oldZ + dz)) {
        pos.z += dz;
        moved = true;
    } else {
        cam_.velocity().z = 0.0f;
    }
    return moved;
}

void Player::update(const World& world, const bool* keys, float dt) {
    bool runKey    = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    bool crouchKey = keys[SDL_SCANCODE_LCTRL]  || keys[SDL_SCANCODE_RCTRL];
    bool slowKey   = keys[SDL_SCANCODE_C];

    float speed, targetH;
    if (crouchKey) {
        mode_ = Mode::Crouch;
        speed = 1.2f; targetH = CROUCH_HEIGHT;
    } else if (slowKey) {
        mode_ = Mode::Slow;
        speed = 1.5f; targetH = STAND_HEIGHT;
    } else if (runKey) {
        mode_ = Mode::Run;
        speed = 6.0f; targetH = STAND_HEIGHT;
    } else {
        mode_ = Mode::Walk;
        speed = 3.0f; targetH = STAND_HEIGHT;
    }

    cam_.setTargetHeight(targetH);
    cam_.updateHeight(dt);

    float inputForward = 0.0f;
    float inputStrafe  = 0.0f;
    if (keys[SDL_SCANCODE_W]) inputForward += 1.0f;
    if (keys[SDL_SCANCODE_S]) inputForward -= 1.0f;
    if (keys[SDL_SCANCODE_D]) inputStrafe  += 1.0f;
    if (keys[SDL_SCANCODE_A]) inputStrafe  -= 1.0f;

    float inputLen = std::sqrt(inputForward * inputForward +
                                inputStrafe  * inputStrafe);
    if (inputLen > 0.001f) {
        inputForward /= inputLen;
        inputStrafe  /= inputLen;
    }

    glm::vec3 front = cam_.front();
    glm::vec3 right = cam_.right();

    glm::vec3 walkFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 walkRight = glm::normalize(glm::vec3(right.x, 0.0f, right.z));

    glm::vec3 targetVel = walkFront * (inputForward * speed)
                        + walkRight * (inputStrafe  * speed);

    float accel = (inputLen > 0.001f) ? 15.0f : 20.0f;
    glm::vec3 delta = targetVel - cam_.velocity();
    float deltaLen = glm::length(delta);
    if (deltaLen > accel * dt) {
        delta = (delta / deltaLen) * accel * dt;
    }
    cam_.velocity() += delta;

    float dx = cam_.velocity().x * dt;
    float dz = cam_.velocity().z * dt;
    tryMove(world, dx, dz);

    float horizSpeed = std::sqrt(cam_.velocity().x * cam_.velocity().x +
                                  cam_.velocity().z * cam_.velocity().z);
    bool moving = horizSpeed > 0.5f;
    bool running = (mode_ == Mode::Run);
    cam_.updateBob(dt, moving && mode_ != Mode::Crouch, running);
}