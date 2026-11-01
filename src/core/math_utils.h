// src/core/math_utils.h
#pragma once

#include <cmath>
#include <algorithm>

template<typename T>
inline T clamp(T value, T minVal, T maxVal) {
    return std::max(minVal, std::min(value, maxVal));
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float deg2rad(float deg) {
    return deg * 3.14159265358979323846f / 180.0f;
}

inline float rad2deg(float rad) {
    return rad * 180.0f / 3.14159265358979323846f;
}

constexpr float PI = 3.14159265358979323846f;

inline float wrapAngle(float angle) {
    while (angle >= 2.0f * PI) angle -= 2.0f * PI;
    while (angle < 0.0f)        angle += 2.0f * PI;
    return angle;
}

inline float angleDelta(float from, float to) {
    float d = to - from;
    while (d >  PI) d -= 2.0f * PI;
    while (d < -PI) d += 2.0f * PI;
    return d;
}