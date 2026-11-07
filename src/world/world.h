#pragma once

#include "core/types.h"
#include "world/chunk.h"
#include "core/save_system.h"

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <map>
#include <tuple>

class World {
public:
    explicit World(unsigned int seed);

    bool isSolid(int wx, int wy, int wz) const;
    bool isWall(float fx, float fz) const;

    Chunk& ensureChunk(int cx, int cy);
    bool setBlock(int wx, int wy, int wz, uint8_t blockType);
    void applyModifiedBlocks(const std::vector<ModifiedBlock>& blocks);
    std::vector<ModifiedBlock> modifiedBlocks() const;
    void loadAround(float wx, float wy, int radius = VIEW_RADIUS);
    void unloadFarFrom(int ccx, int ccy, int radius = VIEW_RADIUS);

    const Chunk* getChunk(int cx, int cy) const;
    const std::unordered_map<uint64_t, Chunk>& chunks() const { return chunks_; }

    unsigned int seed() const { return worldSeed_; }
    static uint64_t key(int cx, int cy);

    std::vector<Light> collectLights(float wx, float wy, float maxDist = 20.0f,
                                     int maxCount = MAX_LIGHTS) const;

    bool findSpawn(float startX, float startY, float& outX, float& outY) const;

private:
    unsigned int worldSeed_;
    std::unordered_map<uint64_t, Chunk> chunks_;
    std::map<std::tuple<int32_t,int32_t,int32_t>,uint8_t> modifiedBlocks_;

    // Lamp list and shadow polygons survive across frames.
    mutable bool lightsDirty_ = true;
    mutable std::vector<Light> cachedLights_;
};