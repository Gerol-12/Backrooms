// src/world/chunk.h
#pragma once

#include <cstdint>
#include <vector>
#include "core/types.h"

// ============================================================
//  Chunk — 32×32 клеток пола/стен
// ============================================================

struct Chunk {
    int cx = 0;
    int cy = 0;
    std::vector<uint8_t> cells;

    Chunk() = default;
    Chunk(int _cx, int _cy)
    : cx(_cx), cy(_cy), cells(CHUNK_SIZE * CHUNK_SIZE, 0) {}

    uint8_t& at(int x, int y) { return cells[(size_t)y * CHUNK_SIZE + x]; }
    uint8_t at(int x, int y) const { return cells[(size_t)y * CHUNK_SIZE + x]; }

bool isFloor(int x, int y) const { return at(x, y) == 1; }
bool isWall(int x, int y) const { return at(x, y) == 0; }
};

unsigned int chunkSeed(unsigned int worldSeed, int cx, int cy);
BiomeType getBiomeAtCell(int wx, int wy, unsigned int worldSeed);
Chunk generateChunk(unsigned int worldSeed, int cx, int cy);