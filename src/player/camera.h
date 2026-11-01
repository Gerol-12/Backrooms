#pragma once
#include <glm/glm.hpp>
#include "core/types.h"

class Camera {
public:
    Camera();
    const glm::vec3& position() const { return pos_; }
    glm::vec3&       position()       { return pos_; }
    float yaw()   const { return yaw_; }
    float pitch() const { return pitch_; }
    void setYaw(float y);
    void setPitch(float p);
    void addYawPitch(float dYaw, float dPitch);
    glm::vec3 front() const;
    glm::vec3 right() const;
    glm::mat4 viewMatrix() const;
    const glm::vec3& velocity() const { return velocity_; }
    glm::vec3&       velocity()       { return velocity_; }
    float currentHeight() const { return currentHeight_; }
    float targetHeight()  const { return targetHeight_; }
    void setTargetHeight(float h) { targetHeight_ = h; }
    void updateHeight(float dt);
    void updateBob(float dt, bool moving, bool running);
    glm::vec3 eyePosition() const;

private:
    glm::vec3 pos_ = glm::vec3(16.5f, 1.0f, 16.5f); // Y=1 ноги
    glm::vec3 velocity_ = glm::vec3(0.0f);
    float yaw_   = -90.0f;
    float pitch_ = 0.0f;
    float targetHeight_  = STAND_HEIGHT;
    float currentHeight_ = STAND_HEIGHT;
    float bobPhase_  = 0.0f;
    float bobAmount_ = 0.0f;
};