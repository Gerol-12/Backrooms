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
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        const Chunk &c = it->second;
        if (std::abs(c.cx - ccx) > VIEW_RADIUS || std::abs(c.cy - ccy) > VIEW_RADIUS) {
            it = chunks_.erase(it);
        } else {
            ++it;
        }
    }
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
    std::vector<Light> result;
    const float maxDist2 = maxDist * maxDist;

    for (const auto &kv : chunks_) {
        const Chunk &c = kv.second;
        if (getBiomeForChunk(c.cx, c.cy, worldSeed_) == BiomeType::DarkRoom) continue;

        int baseX = c.cx * CHUNK_SIZE;
        int baseZ = c.cy * CHUNK_SIZE;

        for (int lz = 3; lz < CHUNK_SIZE - 3; lz += 6) {
            for (int lx = 3; lx < CHUNK_SIZE - 3; lx += 6) {
                // Проверяем, есть ли потолок на высоте 4
                if (!c.isSolid(lx, 4, lz)) continue;

                float px = (float)(baseX + lx) + 0.5f;
                float pz = (float)(baseZ + lz) + 0.5f;
                float dx = px - wx, dz = pz - wy;
                if (dx * dx + dz * dz > maxDist2) continue;

                Light L;
                // Лампа висит под потолком (Y=3.95)
                L.position = glm::vec3(px, 3.95f, pz);
                L.color = glm::vec3(1.0f, 0.92f, 0.65f);
                L.radius = 9.0f;
                L.intensity = 1.6f;
                L.flicker = 1.0f;
                result.push_back(L);
            }
        }
    }

    std::sort(result.begin(), result.end(), [wx, wy](const Light &a, const Light &b) {
        float ax = a.position.x - wx, az = a.position.z - wy;
        float bx = b.position.x - wx, bz = b.position.z - wy;
        return (ax * ax + az * az) < (bx * bx + bz * bz);
    });

    if ((int)result.size() > MAX_LIGHTS) result.resize(MAX_LIGHTS);
    for (auto &L : result) buildLightPolygon(*this, L);

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