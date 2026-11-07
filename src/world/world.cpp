// src/world/world.cpp

#include "world/world.h"
#include "core/math_utils.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>

World::World(unsigned int seed) : worldSeed_(seed) {}

uint64_t World::key(int cx, int cy) {
    return ((uint64_t)(uint32_t)cx << 32) | (uint32_t)cy;
}

const Chunk *World::getChunk(int cx, int cy) const {
    auto it = chunks_.find(key(cx, cy));
    if (it == chunks_.end()) return nullptr;
    return &it->second;
}

bool World::isSolid(int wx, int wy, int wz) const {
    if (wy < 0 || wy >= CHUNK_HEIGHT) return false;

    int cx = (int)std::floor((float)wx / CHUNK_SIZE);
    int cz = (int)std::floor((float)wz / CHUNK_SIZE);
    auto it = chunks_.find(key(cx, cz));

    if (it == chunks_.end()) return true;

    int lx = wx - cx * CHUNK_SIZE;
    int lz = wz - cz * CHUNK_SIZE;
    return it->second.isSolid(lx, wy, lz);
}

bool World::isWall(float fx, float fz) const {
    return isSolid((int)std::floor(fx), 1, (int)std::floor(fz));
}

Chunk &World::ensureChunk(int cx, int cy) {
    uint64_t k = key(cx, cy);
    auto it = chunks_.find(k);
    if (it != chunks_.end()) return it->second;

    Chunk c = generateChunk(worldSeed_, cx, cy);
    auto res = chunks_.emplace(k, std::move(c));
    lightsDirty_ = true;
    return res.first->second;
}

void World::loadAround(float wx, float wy) {
    int ccx = (int)std::floor(wx / CHUNK_SIZE);
    int ccy = (int)std::floor(wy / CHUNK_SIZE);
    for (int dy = -VIEW_RADIUS; dy <= VIEW_RADIUS; ++dy) {
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; ++dx) {
            ensureChunk(ccx + dx, ccy + dy);
        }
    }
}

void World::unloadFarFrom(int ccx, int ccy) {
    bool removed = false;

    for (auto it = chunks_.begin(); it != chunks_.end();) {
        const Chunk &c = it->second;
        if (std::abs(c.cx - ccx) > VIEW_RADIUS || std::abs(c.cy - ccy) > VIEW_RADIUS) {
            it = chunks_.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }

    if (removed)
        lightsDirty_ = true;
}

static void buildLightPolygon(const World &world, Light &L) {
    const int N = MAX_POLY_POINTS;
    L.polyCount = N;

    for (int i = 0; i < N; ++i) {
        float angle = (2.0f * PI * (float)i) / (float)N;
        float dx = std::cos(angle);
        float dz = std::sin(angle);

        int cx = (int)std::floor(L.position.x);
        int cz = (int)std::floor(L.position.z);

        int stepX = (dx > 0.0f) ? 1 : ((dx < 0.0f) ? -1 : 0);
        int stepZ = (dz > 0.0f) ? 1 : ((dz < 0.0f) ? -1 : 0);

        float tMaxX = (stepX != 0) ? std::abs(((stepX > 0 ? cx + 1.0f : (float)cx) - L.position.x) / dx) : 1e30f;
        float tMaxZ = (stepZ != 0) ? std::abs(((stepZ > 0 ? cz + 1.0f : (float)cz) - L.position.z) / dz) : 1e30f;
        float tDeltaX = (stepX != 0) ? std::abs(1.0f / dx) : 1e30f;
        float tDeltaZ = (stepZ != 0) ? std::abs(1.0f / dz) : 1e30f;

        float hitDist = L.radius;
        float t = 0.0f;

        while (t < L.radius) {
            // Проверяем стены на высоте 1
            if (world.isSolid(cx, 1, cz)) {
                hitDist = std::min(t + 0.05f, L.radius);
                break;
            }
            if (tMaxX < tMaxZ) {
                t = tMaxX; cx += stepX; tMaxX += tDeltaX;
            } else {
                t = tMaxZ; cz += stepZ; tMaxZ += tDeltaZ;
            }
        }
        L.polyDist[i] = hitDist;
    }
}

std::vector<Light> World::collectLights(float wx, float wy, float maxDist) const {
    // Rebuild the lamp index only when the loaded world changes.
    if (lightsDirty_) {
        cachedLights_.clear();

        for (const auto &kv : chunks_) {
            const Chunk &c = kv.second;
            const int baseX = c.cx * CHUNK_SIZE;
            const int baseZ = c.cy * CHUNK_SIZE;

            for (int lz = 0; lz < CHUNK_SIZE; ++lz) {
                for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
                    if (c.at(lx, 4, lz) != 2)
                        continue;

                    const float px = static_cast<float>(baseX + lx) + 0.5f;
                    const float pz = static_cast<float>(baseZ + lz) + 0.5f;

                    Light light{};
                    light.position = glm::vec3(px, 3.95f, pz);
                    light.color = glm::vec3(1.0f, 0.92f, 0.65f);
                    light.radius = 15.0f;
                    light.intensity = 2.5f;
                    light.flicker = 1.0f;
                    light.polyCount = 0; // Zero means that the DDA polygon is not built yet.

                    cachedLights_.push_back(light);
                }
            }
        }

        lightsDirty_ = false;
    }

    // Select the nearest lights each frame, without rescanning every block.
    const float maxDist2 = maxDist * maxDist;
    std::vector<size_t> candidates;
    candidates.reserve(cachedLights_.size());

    for (size_t i = 0; i < cachedLights_.size(); ++i) {
        const Light &light = cachedLights_[i];
        const float dx = light.position.x - wx;
        const float dz = light.position.z - wy;

        if (dx * dx + dz * dz <= maxDist2)
            candidates.push_back(i);
    }

    const auto distanceLess = [&](size_t a, size_t b) {
        const glm::vec3 &A = cachedLights_[a].position;
        const glm::vec3 &B = cachedLights_[b].position;
        const float ax = A.x - wx, az = A.z - wy;
        const float bx = B.x - wx, bz = B.z - wy;
        return ax * ax + az * az < bx * bx + bz * bz;
    };

    const size_t count = std::min(
        candidates.size(),
        static_cast<size_t>(MAX_LIGHTS)
    );

    if (count == 0)
        return {};

    if (candidates.size() > count) {
        std::partial_sort(
            candidates.begin(),
            candidates.begin() + count,
            candidates.end(),
            distanceLess
        );
    } else {
        std::sort(candidates.begin(), candidates.end(), distanceLess);
    }

    std::vector<Light> result;
    result.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        Light &light = cachedLights_[candidates[i]];

        // DDA shadow rays are traced once per cached lamp.
        if (light.polyCount == 0)
            buildLightPolygon(*this, light);

        result.push_back(light);
    }

    return result;
}

bool World::findSpawn(float startX, float startY, float &outX, float &outY) const {
    int baseX = (int)std::floor(startX);
    int baseZ = (int)std::floor(startY);

    for (int r = 0; r < 64; ++r) {
        for (int dz = -r; dz <= r; ++dz) {
            for (int dx = -r; dx <= r; ++dx) {
                if (r > 0 && std::abs(dx) != r && std::abs(dz) != r) continue;
                int cx = baseX + dx, cz = baseZ + dz;
                if (isSolid(cx, 0, cz) && !isSolid(cx, 1, cz) && !isSolid(cx, 2, cz)) {
                    outX = (float)cx + 0.5f; outY = (float)cz + 0.5f; return true;
                }
            }
        }
    }
    return false;
}