// src/world/chunk.h
#pragma once

#include <cstdint>
#include <vector>
#include "core/types.h"

constexpr int CHUNK_HEIGHT = 32;

struct Chunk {
    int cx = 0;
    int cy = 0;
    std::vector<uint8_t> blocks;

    Chunk() = default;
    Chunk(int _cx, int _cy)
    : cx(_cx), cy(_cy), blocks(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE, 0) {}

    uint8_t& at(int x, int y, int z) {
        return blocks[(size_t)y * (CHUNK_SIZE * CHUNK_SIZE) + (size_t)z * CHUNK_SIZE + x];
    }
    uint8_t at(int x, int y, int z) const {
        return blocks[(size_t)y * (CHUNK_SIZE * CHUNK_SIZE) + (size_t)z * CHUNK_SIZE + x];
    }

    // 0 - воздух, 1 - обычный блок, 2 - лампа
    bool isSolid(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) return false;
        return at(x, y, z) > 0;
    }
};

unsigned int chunkSeed(unsigned int worldSeed, int cx, int cy);
BiomeType getBiomeForChunk(int cx, int cy, unsigned int worldSeed);
Chunk generateChunk(unsigned int worldSeed, int cx, int cy);