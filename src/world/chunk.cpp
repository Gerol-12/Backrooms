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

// ============================================================
// 1. ВЫБОР БИОМА ДЛЯ ВСЕГО ЧАНКА
// ============================================================
BiomeType getBiomeForChunk(int cx, int cy, unsigned int worldSeed) {
    // Масштаб 0.2f означает, что биомы будут меняться каждые 5 чанков
    const float scale = 0.2f;
    float n = valueNoise(cx * scale, cy * scale, worldSeed ^ 0x12345u);

    if (n < 0.30f) return BiomeType::OpenHall;
    if (n < 0.50f) return BiomeType::Pillars;
    if (n < 0.70f) return BiomeType::LongCorridors;
    if (n < 0.90f) return BiomeType::CorridorMaze;
    return BiomeType::DarkRoom;
}

// ============================================================
// 2. ЛОГИКА ГЕНЕРАЦИИ БЛОКОВ (ПО СЛОЯМ Y)
// ============================================================
static int getBlockType(BiomeType b, int wx, int wy, int y, unsigned seed) {

    // --- ВЕРТИКАЛЬНЫЕ ПРАВИЛА ---
    if (y < 0 || y > 4) return 0; // Выше потолка и ниже пола — пустота
    if (y == 0) return 1;         // Пол
    if (y == 4) {
        // Защита от отрицательных координат (C++ баг с %)
        int lx = ((wx % 6) + 6) % 6;
        int lz = ((wy % 6) + 6) % 6;

        // Ставим лампу, если остаток равен 3 (и это не темная комната)
        if (lx == 3 && lz == 3 && b != BiomeType::DarkRoom) {
            return 2; // Лампа
        }
        return 1; // Обычная плитка потолка
    }


    // --- ГОРИЗОНТАЛЬНЫЕ ПРАВИЛА (Y от 1 до 3) ---
    bool isWall = false;

    // Размер "комнаты" в блоках. Сейчас 12х12.
    int roomSize = 12;
    int localX = ((wx % roomSize) + roomSize) % roomSize;
    int localZ = ((wy % roomSize) + roomSize) % roomSize;

    // Логика для разных биомов
    switch (b) {
        case BiomeType::OpenHall: {
            // Сетка комнат 12x12, но дверные проёмы зависят от seed.
            // Решение привязано к мировым координатам, поэтому не зависит
            // от того, в каком чанке генерируется участок.
            if (localX == 0) {
                const int roomX = (int)std::floor((float)wx / roomSize);
                const int roomZ = (int)std::floor((float)wy / roomSize);
                const bool doorway = hash01(roomX, roomZ, seed ^ 0x51A7u) > 0.28f;
                const bool inOpening = localZ >= 5 && localZ <= 6;
                isWall = !(doorway && inOpening);
            } else if (localZ == 0) {
                const int roomX = (int)std::floor((float)wx / roomSize);
                const int roomZ = (int)std::floor((float)wy / roomSize);
                const bool doorway = hash01(roomX, roomZ, seed ^ 0xA715u) > 0.28f;
                const bool inOpening = localX >= 5 && localX <= 6;
                isWall = !(doorway && inOpening);
            }
            break;
        }
        case BiomeType::Pillars: {
            // Огромный зал. Стены только на границах чанков.
            if (localX == 0 || localZ == 0) isWall = true;

            // Ставим колонны 2х2 в центре каждой комнаты
            if ((localX == 5 || localX == 6) && (localZ == 5 || localZ == 6)) {
                isWall = true;
            }
            break;
        }
        case BiomeType::LongCorridors: {
            // Только горизонтальные коридоры (стены сверху и снизу)
            if (localZ == 0 || localZ == 1 || localZ == 6 || localZ == 7) {
                isWall = true;
            }
            break;
        }
        case BiomeType::CorridorMaze: {
            // Коридорная сетка с переменными проёмами.
            // Стены идут по сетке 8x8, а решения о проходах вычисляются
            // из мировых координат: результат одинаков на границах чанков.
            const int mazeSize = 8;
            const int mx = ((wx % mazeSize) + mazeSize) % mazeSize;
            const int mz = ((wy % mazeSize) + mazeSize) % mazeSize;

            if (mx == 0) {
                const int wallX = (int)std::floor((float)wx / mazeSize);
                const int segmentZ = (int)std::floor((float)wy / mazeSize);
                const bool doorway = hash01(wallX, segmentZ, seed ^ 0xC031u) > 0.42f;
                isWall = !(doorway && mz >= 3 && mz <= 4);
            } else if (mz == 0) {
                const int segmentX = (int)std::floor((float)wx / mazeSize);
                const int wallZ = (int)std::floor((float)wy / mazeSize);
                const bool doorway = hash01(segmentX, wallZ, seed ^ 0xC032u) > 0.42f;
                isWall = !(doorway && mx >= 3 && mx <= 4);
            }
            break;
        }
        case BiomeType::DarkRoom: {
            // Пустая комната. Только стены на границах чанка (каждые 32 блока)
            if (wx % CHUNK_SIZE == 0 || wy % CHUNK_SIZE == 0) isWall = true;
            break;
        }
    }

    return isWall ? 1 : 0;
}

// ============================================================
// 3. ГЛАВНАЯ ФУНКЦИЯ ГЕНЕРАЦИИ
// ============================================================
Chunk generateChunk(unsigned int worldSeed, int cx, int cy) {
    Chunk c(cx, cy);
    int baseX = cx * CHUNK_SIZE;
    int baseZ = cy * CHUNK_SIZE;

    BiomeType biome = getBiomeForChunk(cx, cy, worldSeed);

    for (int z = 0; z < CHUNK_SIZE; ++z) {
        for (int x = 0; x < CHUNK_SIZE; ++x) {
            int wx = baseX + x;
            int wz = baseZ + z;

            for (int y = 0; y < CHUNK_HEIGHT; ++y) {
                c.at(x, y, z) = getBlockType(biome, wx, wz, y, worldSeed);
            }
        }
    }

    // --- ГАРАНТИРОВАННЫЕ ДВЕРИ НА СТЫКАХ ЧАНКОВ ---
    // Это нужно, чтобы соседние чанки (даже разных биомов) всегда соединялись.
    for (int x = 0; x < CHUNK_SIZE; ++x) {
        // Проходы на северной и южной границе (Z = 0 и Z = 31)
        if (x % 8 == 4 || x % 8 == 5) {
            c.at(x, 1, 0) = 0; c.at(x, 2, 0) = 0; c.at(x, 3, 0) = 0;
            c.at(x, 1, CHUNK_SIZE - 1) = 0; c.at(x, 2, CHUNK_SIZE - 1) = 0; c.at(x, 3, CHUNK_SIZE - 1) = 0;
        }
    }
    for (int z = 0; z < CHUNK_SIZE; ++z) {
        // Проходы на западной и восточной границе (X = 0 и X = 31)
        if (z % 8 == 4 || z % 8 == 5) {
            c.at(0, 1, z) = 0; c.at(0, 2, z) = 0; c.at(0, 3, z) = 0;
            c.at(CHUNK_SIZE - 1, 1, z) = 0; c.at(CHUNK_SIZE - 1, 2, z) = 0; c.at(CHUNK_SIZE - 1, 3, z) = 0;
        }
    }

    return c;
}