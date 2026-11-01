#include "player/player.h"
#include "core/math_utils.h"
#include <SDL3/SDL.h>
#include <cmath>

Player::Player() = default;
const char* Player::modeName() const {
    switch (mode_) {
        case Mode::Walk: return "WALK";
        case Mode::Run: return "RUN";
        case Mode::Slow: return "SLOW";
        case Mode::Crouch: return "CROUCH";
    }
    return "?";
}

bool Player::checkCollision(const World& world, float x, float y, float z, float height) const {
    float r = PLAYER_RADIUS;
    float minX = x - r, maxX = x + r;
    float minY = y + 0.1f, maxY = y + height - 0.1f;
    float minZ = z - r, maxZ = z + r;
    for (int by = (int)std::floor(minY); by <= (int)std::floor(maxY); ++by)
        for (int bx = (int)std::floor(minX); bx <= (int)std::floor(maxX); ++bx)
            for (int bz = (int)std::floor(minZ); bz <= (int)std::floor(maxZ); ++bz)
                if (world.isSolid(bx, by, bz)) return true;
    return false;
}

void Player::update(const World& world, const bool* keys, float dt) {
    bool runKey    = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    bool crouchKey = keys[SDL_SCANCODE_LCTRL]  || keys[SDL_SCANCODE_RCTRL];
    bool slowKey   = keys[SDL_SCANCODE_C];

    float speed, targetH;
    if (crouchKey)      { mode_ = Mode::Crouch; speed = 1.2f; targetH = CROUCH_HEIGHT; }
    else if (slowKey)   { mode_ = Mode::Slow;   speed = 1.5f; targetH = STAND_HEIGHT; }
    else if (runKey)    { mode_ = Mode::Run;    speed = 6.0f; targetH = STAND_HEIGHT; }
    else                { mode_ = Mode::Walk;   speed = 3.0f; targetH = STAND_HEIGHT; }

    cam_.setTargetHeight(targetH);
    cam_.updateHeight(dt);

    if (keys[SDL_SCANCODE_SPACE] && isGrounded) { cam_.velocity().y = 8.0f; isGrounded = false; }
    cam_.velocity().y -= gravity * dt;

    float inputForward = (keys[SDL_SCANCODE_W] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_S] ? 1.0f : 0.0f);
    float inputStrafe  = (keys[SDL_SCANCODE_D] ? 1.0f : 0.0f) - (keys[SDL_SCANCODE_A] ? 1.0f : 0.0f);
    float inputLen = std::sqrt(inputForward * inputForward + inputStrafe * inputStrafe);
    if (inputLen > 0.001f) { inputForward /= inputLen; inputStrafe /= inputLen; }

    glm::vec3 front = cam_.front(); glm::vec3 right = cam_.right();
    glm::vec3 walkFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 walkRight = glm::normalize(glm::vec3(right.x, 0.0f, right.z));
    glm::vec3 targetVel = walkFront * (inputForward * speed) + walkRight * (inputStrafe * speed);

    float accel = (inputLen > 0.001f) ? 15.0f : 20.0f;
    glm::vec3 delta = targetVel - cam_.velocity(); delta.y = 0;
    float deltaLen = glm::length(delta);
    if (deltaLen > accel * dt) delta = (delta / deltaLen) * accel * dt;
    cam_.velocity().x += delta.x; cam_.velocity().z += delta.z;

    glm::vec3& pos = cam_.position();

    pos.x += cam_.velocity().x * dt;
    if (checkCollision(world, pos.x, pos.y, pos.z, targetH)) { pos.x -= cam_.velocity().x * dt; cam_.velocity().x = 0; }

    pos.z += cam_.velocity().z * dt;
    if (checkCollision(world, pos.x, pos.y, pos.z, targetH)) { pos.z -= cam_.velocity().z * dt; cam_.velocity().z = 0; }

    pos.y += cam_.velocity().y * dt;
    if (checkCollision(world, pos.x, pos.y, pos.z, targetH)) {
        pos.y -= cam_.velocity().y * dt;
        if (cam_.velocity().y < 0) isGrounded = true;
        cam_.velocity().y = 0;
    } else isGrounded = false;

    if (pos.y < -10.0f) { pos = glm::vec3(16.5f, 5.0f, 16.5f); cam_.velocity() = glm::vec3(0.0f); }

    float horizSpeed = std::sqrt(cam_.velocity().x * cam_.velocity().x + cam_.velocity().z * cam_.velocity().z);
    cam_.updateBob(dt, horizSpeed > 0.5f && mode_ != Mode::Crouch, (mode_ == Mode::Run));
}