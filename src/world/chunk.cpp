// src/world/chunk.cpp

#include "world/chunk.h"
#include <random>
#include <cmath>
#include <cstdint>

// ============================================================
//  Сид чанка
// ============================================================

unsigned int chunkSeed(unsigned int worldSeed, int cx, int cy) {
    uint32_t h = (uint32_t)worldSeed;
    h ^= (uint32_t)cx * 374761393u;
    h ^= (uint32_t)cy * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// ============================================================
//  Хеш-функция [0, 1] по (x, y)
// ============================================================

static float hash01(int x, int y, unsigned seed) {
    uint32_t h = seed;
    h ^= (uint32_t)x * 374761393u;
    h ^= (uint32_t)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return (h & 0xFFFFu) / 65535.0f;
}

static float smoothstepf(float t) {
    return t * t * (3.0f - 2.0f * t);
}

// ============================================================
//  Value Noise 2D — плавный шум
// ============================================================

static float valueNoise(float x, float y, unsigned seed) {
    int x0 = (int)std::floor(x);
    int y0 = (int)std::floor(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float fx = smoothstepf(x - (float)x0);
    float fy = smoothstepf(y - (float)y0);

    float v00 = hash01(x0, y0, seed);
    float v10 = hash01(x1, y0, seed);
    float v01 = hash01(x0, y1, seed);
    float v11 = hash01(x1, y1, seed);

    float vx0 = v00 + (v10 - v00) * fx;
    float vx1 = v01 + (v11 - v01) * fx;
    return vx0 + (vx1 - vx0) * fy;
}

// ============================================================
//  Определение биома для клетки — через шум
// ============================================================

BiomeType getBiomeAtCell(int wx, int wy, unsigned int worldSeed) {
    const float scale = 1.0f / 18.0f;
    float n = valueNoise(wx * scale, wy * scale, worldSeed ^ 0x12345u);

    if (n < 0.14f) return BiomeType::VastEmpty;
    if (n < 0.32f) return BiomeType::OpenHall;
    if (n < 0.48f) return BiomeType::Pillars;
    if (n < 0.62f) return BiomeType::LongCorridors;
    if (n < 0.82f) return BiomeType::CorridorMaze;
    return BiomeType::Cramped;
}

// ============================================================
//  Правило: пол или стена для клетки в её биоме
// ============================================================

static bool isFloorInBiome(BiomeType b, int wx, int wy, unsigned seed) {
    switch (b) {
    case BiomeType::VastEmpty:
            return true;

        case BiomeType::OpenHall: {
            int gx = ((wx % 8) + 8) % 8;
            int gy = ((wy % 8) + 8) % 8;
            return !(gx == 0 && gy == 0);
        }

        case BiomeType::Pillars: {
            int gx = ((wx % 14) + 14) % 14;
            int gy = ((wy % 14) + 14) % 14;
            bool pillar = (gx <= 1) && (gy <= 1);
            return !pillar;
        }

        case BiomeType::LongCorridors: {
            int gx = ((wx % 9) + 9) % 9;
            int gy = ((wy % 9) + 9) % 9;
            bool hC = (gy < 2);
            bool vC = (gx < 2);
            return hC || vC;
        }

         case BiomeType::CorridorMaze: {
            int gx = ((wx % 4) + 4) % 4;
            int gy = ((wy % 4) + 4) % 4;
            bool hC = (gy == 0);
            bool vC = (gx == 0);
            bool extra = hash01(wx / 2, wy / 2, seed) > 0.65f;
            return hC || vC || extra;
        }

        case BiomeType::Cramped: {
            bool gridA = (wx % 3 == 0);
            bool gridB = (wy % 3 == 0);
            bool rnd = hash01(wx, wy, seed) < 0.15f;
            return gridA || gridB || rnd;
        }
    }
    return true;
}

// ============================================================
//  Главная функция генерации чанка
// ============================================================

    Chunk generateChunk(unsigned int worldSeed, int cx, int cy) {
    Chunk c(cx, cy);

    int baseX = cx * CHUNK_SIZE;
    int baseY = cy * CHUNK_SIZE;

    for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
        for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
            int wx = baseX + lx;
            int wy = baseY + ly;

            BiomeType b = getBiomeAtCell(wx, wy, worldSeed);
            bool floor = isFloorInBiome(b, wx, wy, worldSeed);
            c.at(lx, ly) = floor ? 1 : 0;
        }
    }

    // Гарантированные проходы на границах чанка
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        bool doorX = (x % 8 == 4) || (x % 8 == 5);
        if (doorX) {
            c.at(x, 0) = 1;
            c.at(x, CHUNK_SIZE - 1) = 1;
        }
    }
    for (int y = 0; y < CHUNK_SIZE; ++y) {
        bool doorY = (y % 8 == 4) || (y % 8 == 5);
        if (doorY) {
            c.at(0, y) = 1;
            c.at(CHUNK_SIZE - 1, y) = 1;
        }
    }

    c.at(0, 0) = 0;
    c.at(CHUNK_SIZE - 1, 0) = 0;
    c.at(0, CHUNK_SIZE - 1) = 0;
    c.at(CHUNK_SIZE - 1, CHUNK_SIZE - 1) = 0;

    return c;
}