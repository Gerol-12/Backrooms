// src/core/types.h
#pragma once

#include <glm/glm.hpp>
#include <cstdint>

// ============================================================
//  Vertex
// ============================================================

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

// ============================================================
//  ChunkPos
// ============================================================

struct ChunkPos {
    int x = 0;
    int y = 0;

    bool operator==(const ChunkPos& o) const { return x == o.x && y == o.y; }
    bool operator!=(const ChunkPos& o) const { return !(*this == o); }
};

// ============================================================
//  Direction
// ============================================================

enum class Direction : uint8_t {
    North = 0,
    South = 1,
    West = 2,
    East = 3
};

// ============================================================
//  BiomeType — тип региона мира
// ============================================================

enum class BiomeType : uint8_t {
    OpenHall = 0,
    CorridorMaze = 1,
    LongCorridors = 2,
    VastEmpty = 3,
    Pillars = 4,
    Cramped = 5
};

constexpr int REGION_SIZE = 4;

// ============================================================
//  Constants
// ============================================================

constexpr int CHUNK_SIZE = 32;
constexpr int VIEW_RADIUS = 1;
constexpr float CEIL_HEIGHT = 2.5f;
constexpr float WALL_TILE = 1.5f;
constexpr float FLOOR_TILE = 2.0f;
constexpr float CEIL_TILE  = 1.0f;
constexpr float PLAYER_RADIUS = 0.3f;
constexpr float STAND_HEIGHT = 1.5f;
constexpr float CROUCH_HEIGHT = 0.8f;

constexpr int MAX_LIGHTS = 12;
constexpr int MAX_POLY_POINTS = 240;

// ============================================================
//  Light
// ============================================================

struct Light {
    glm::vec3 position;
    glm::vec3 color;
    float radius = 6.5f;
    float intensity = 2.0f;
    float flicker = 1.0f;

    int polyCount = 0;
    float polyDist[MAX_POLY_POINTS] = {0};
};