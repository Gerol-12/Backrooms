#pragma once

#include <cstdint>
#include <glm/glm.hpp>

constexpr int CHUNK_SIZE = 32;
constexpr int VIEW_RADIUS = 3;
constexpr float CEIL_HEIGHT = 3.0f;
constexpr float STAND_HEIGHT = 1.6f;
constexpr float CROUCH_HEIGHT = 1.0f;
constexpr float PLAYER_RADIUS = 0.3f;
constexpr int MAX_POLY_POINTS = 240;
constexpr int MAX_LIGHTS = 12;

enum class BiomeType {
    OpenHall,
    Pillars,
    LongCorridors,
    CorridorMaze,
    DarkRoom
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float radius;
    float intensity;
    float flicker;
    int polyCount = 0;
    float polyDist[MAX_POLY_POINTS];
};