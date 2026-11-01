// src/world/chunk.cpp

#include "world/chunk.h"
#include <random>
#include <cmath>
#include <cstdint>

unsigned int chunkSeed(unsigned int worldSeed, int cx, int cy) {
    uint32_t h = (uint32_t)worldSeed;
    h ^= (uint32_t)cx * 374761393u;
    h ^= (uint32_t)cy * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

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

BiomeType getBiomeForChunk(int cx, int cy, unsigned int worldSeed) {
    const float scale = 0.15f;
    float n = valueNoise(cx * scale, cy * scale, worldSeed ^ 0x12345u);

    if (n < 0.20f) return BiomeType::OpenHall;
    if (n < 0.45f) return BiomeType::Pillars;
    if (n < 0.65f) return BiomeType::LongCorridors;
    if (n < 0.85f) return BiomeType::CorridorMaze;
    return BiomeType::DarkRoom;
}

static bool isFloorInBiome(BiomeType b, int wx, int wy, unsigned seed) {
    switch (b) {
        case BiomeType::OpenHall: {
            // Классический Level 0. В основном открытое пространство, но с хаотичными стенами
            float noise = valueNoise(wx * 0.1f, wy * 0.1f, seed);
            if (noise > 0.7f) return false; // Случайная стена
            return true;
        }
        case BiomeType::Pillars: {
            // Зал с колоннами. Смещаем сетку, чтобы не было идеальных квадратов
            int gx = ((wx + 2) % 16 + 16) % 16;
            int gy = ((wy + 2) % 16 + 16) % 16;
            bool pillar = (gx < 3) && (gy < 3);
            return !pillar;
        }
        case BiomeType::LongCorridors: {
            // Длинные коридоры вдоль оси X или Z
            float dir = valueNoise(wx * 0.02f, wy * 0.02f, seed ^ 0xABC);
            if (dir < 0.5f) {
                // Горизонтальные
                int gy = ((wy % 12) + 12) % 12;
                return (gy > 3 && gy < 8);
            } else {
                // Вертикальные
                int gx = ((wx % 12) + 12) % 12;
                return (gx > 3 && gx < 8);
            }
        }
        case BiomeType::CorridorMaze: {
            // Тесный лабиринт
            int gx = ((wx % 6) + 6) % 6;
            int gy = ((wy % 6) + 6) % 6;
            bool hC = (gy == 0 || gy == 1);
            bool vC = (gx == 0 || gx == 1);
            return (hC || vC);
        }
        case BiomeType::DarkRoom: {
            // Темная комната
            bool rnd = hash01(wx, wy, seed) < 0.1f;
            return !rnd;
        }
    }
    return true;
}

Chunk generateChunk(unsigned int worldSeed, int cx, int cy) {
    Chunk c(cx, cy);
    int baseX = cx * CHUNK_SIZE;
    int baseZ = cy * CHUNK_SIZE;

    BiomeType biome = getBiomeForChunk(cx, cy, worldSeed);

    for (int z = 0; z < CHUNK_SIZE; ++z) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            int wx = baseX + x;
            int wz = baseZ + z;
            bool isFloorHere = isFloorInBiome(biome, wx, wz, worldSeed);

            if (!isFloorHere) {
                // Стены высотой 5 блоков
                for (int y = 0; y < 5; ++y) c.at(x, y, z) = 1;
            } else {
                c.at(x, 0, z) = 1; // Пол

                // Высота потолка 4 блока
                c.at(x, 4, z) = 1;

                // Воздух между полом и потолком
                c.at(x, 1, z) = 0;
                c.at(x, 2, z) = 0;
                c.at(x, 3, z) = 0;

                // Случайные препятствия (коробки/столы)
                if (hash01(wx, wz, worldSeed ^ 0xBB) > 0.97f) {
                     c.at(x, 1, z) = 1;
                }
                if (hash01(wx, wz, worldSeed ^ 0xCC) > 0.99f) {
                     c.at(x, 1, z) = 1;
                     c.at(x, 2, z) = 1;
                }
            }
        }
    }

    // Двери на границах (высотой 3 блока)
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        if (x % 8 == 4 || x % 8 == 5) {
            c.at(x, 1, 0) = 0; c.at(x, 2, 0) = 0; c.at(x, 3, 0) = 0;
            c.at(x, 1, CHUNK_SIZE - 1) = 0; c.at(x, 2, CHUNK_SIZE - 1) = 0; c.at(x, 3, CHUNK_SIZE - 1) = 0;
        }
    }
    for (int z = 0; z < CHUNK_SIZE; ++z) {
        if (z % 8 == 4 || z % 8 == 5) {
            c.at(0, 1, z) = 0; c.at(0, 2, z) = 0; c.at(0, 3, z) = 0;
            c.at(CHUNK_SIZE - 1, 1, z) = 0; c.at(CHUNK_SIZE - 1, 2, z) = 0; c.at(CHUNK_SIZE - 1, 3, z) = 0;
        }
    }

    return c;
}