#include "player/camera.h"
#include "core/math_utils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

Camera::Camera() = default;
void Camera::setYaw(float y) { yaw_ = y; }
void Camera::setPitch(float p) { pitch_ = clamp(p, -89.0f, 89.0f); }
void Camera::addYawPitch(float dYaw, float dPitch) {
    yaw_ += dYaw; pitch_ = clamp(pitch_ + dPitch, -89.0f, 89.0f);
    if (yaw_ >  100000.0f) yaw_ -= 2.0f * PI * std::floor(yaw_ / (2.0f * PI));
    if (yaw_ < -100000.0f) yaw_ += 2.0f * PI * std::floor(-yaw_ / (2.0f * PI));
}
glm::vec3 Camera::front() const {
    float yr = deg2rad(yaw_), pr = deg2rad(pitch_);
    return glm::normalize(glm::vec3(std::cos(yr) * std::cos(pr), std::sin(pr), std::sin(yr) * std::cos(pr)));
}
glm::vec3 Camera::right() const { return glm::normalize(glm::cross(front(), glm::vec3(0.0f, 1.0f, 0.0f))); }
glm::mat4 Camera::viewMatrix() const { return glm::lookAt(eyePosition(), eyePosition() + front(), glm::vec3(0.0f, 1.0f, 0.0f)); }
void Camera::updateHeight(float dt) { currentHeight_ += (targetHeight_ - currentHeight_) * 8.0f * dt; }
void Camera::updateBob(float dt, bool moving, bool running) {
    if (moving) { float freq = running ? 12.0f : 8.0f; bobPhase_ += dt * freq; bobAmount_ = std::min(bobAmount_ + dt * 4.0f, 1.0f); }
    else { bobAmount_ = std::max(bobAmount_ - dt * 5.0f, 0.0f); }
    if (bobPhase_ > 6.28318f * 4.0f) bobPhase_ -= 6.28318f * 4.0f;
}
glm::vec3 Camera::eyePosition() const { return glm::vec3(pos_.x, pos_.y + currentHeight_ + std::sin(bobPhase_) * bobAmount_ * 0.05f, pos_.z); }