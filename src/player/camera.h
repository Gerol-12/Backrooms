// src/player/camera.h
#pragma once

#include <glm/glm.hpp>
#include "core/types.h"

// ============================================================
//  Camera — позиция, углы, движение
// ============================================================

class Camera {
public:
    Camera();

    // === Доступ ===
    const glm::vec3& position() const { return pos_; }
    glm::vec3&       position()       { return pos_; }

    float yaw()   const { return yaw_; }
    float pitch() const { return pitch_; }

    void setYaw(float y);
    void setPitch(float p);

    // === Углы через мышь ===
    void addYawPitch(float dYaw, float dPitch);

    // === Направления ===
    glm::vec3 front() const;
    glm::vec3 right() const;

    // === Матрица вида ===
    glm::mat4 viewMatrix() const;

    // === Скорость ===
    const glm::vec3& velocity() const { return velocity_; }
    glm::vec3&       velocity()       { return velocity_; }

    // === Высота камеры ===
    float currentHeight() const { return currentHeight_; }
    float targetHeight()  const { return targetHeight_; }

    void setTargetHeight(float h) { targetHeight_ = h; }
    void updateHeight(float dt);

    // === Head bob ===
    float bobPhase()  const { return bobPhase_; }
    float bobAmount() const { return bobAmount_; }

    void updateBob(float dt, bool moving, bool running);

    // === Финальная позиция камеры (с учётом bob) ===
    glm::vec3 eyePosition() const;

private:
    glm::vec3 pos_ = glm::vec3(16.5f, STAND_HEIGHT, 16.5f);
    glm::vec3 velocity_ = glm::vec3(0.0f);
    float yaw_   = -90.0f;
    float pitch_ = 0.0f;

    float targetHeight_  = STAND_HEIGHT;
    float currentHeight_ = STAND_HEIGHT;

    float bobPhase_  = 0.0f;
    float bobAmount_ = 0.0f;
};