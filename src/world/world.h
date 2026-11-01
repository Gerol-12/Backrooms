// src/world/world.h
#pragma once

#include "core/types.h"
#include "world/chunk.h"

#include <unordered_map>
#include <vector>
#include <cstdint>

class World {
public:
    explicit World(unsigned int seed);

    bool isFloor(int wx, int wy) const;
    bool isWall(float fx, float fy) const;

    Chunk& ensureChunk(int cx, int cy);
    void loadAround(float wx, float wy);
    void unloadFarFrom(int ccx, int ccy);

    const Chunk* getChunk(int cx, int cy) const;

    const std::unordered_map<uint64_t, Chunk>& chunks() const { return chunks_; }

    unsigned int seed() const { return worldSeed_; }

    static uint64_t key(int cx, int cy);

    std::vector<Light> collectLights(float wx, float wy,
                                     float maxDist = 20.0f) const;

    bool findSpawn(float startX, float startY, float& outX, float& outY) const;

private:
    unsigned int worldSeed_;
    std::unordered_map<uint64_t, Chunk> chunks_;
};