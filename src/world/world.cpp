// src/world/world.cpp

#include "world/world.h"
#include "core/math_utils.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>

World::World(unsigned int seed)
    : worldSeed_(seed)
{
}

uint64_t World::key(int cx, int cy)
{
    return ((uint64_t)(uint32_t)cx << 32) | (uint32_t)cy;
}

const Chunk *World::getChunk(int cx, int cy) const
{
    auto it = chunks_.find(key(cx, cy));
    if (it == chunks_.end())
        return nullptr;
    return &it->second;
}

bool World::isFloor(int wx, int wy) const
{
    int cx = (int)std::floor((float)wx / CHUNK_SIZE);
    int cy = (int)std::floor((float)wy / CHUNK_SIZE);
    auto it = chunks_.find(key(cx, cy));
    if (it == chunks_.end())
        return false;

    int lx = wx - cx * CHUNK_SIZE;
    int ly = wy - cy * CHUNK_SIZE;
    return it->second.isFloor(lx, ly);
}

bool World::isWall(float fx, float fy) const
{
    int x = (int)std::floor(fx);
    int y = (int)std::floor(fy);
    return !isFloor(x, y);
}

Chunk &World::ensureChunk(int cx, int cy)
{
    uint64_t k = key(cx, cy);
    auto it = chunks_.find(k);
    if (it != chunks_.end())
        return it->second;

    Chunk c = generateChunk(worldSeed_, cx, cy);
    auto res = chunks_.emplace(k, std::move(c));
    return res.first->second;
}

void World::loadAround(float wx, float wy)
{
    int ccx = (int)std::floor(wx / CHUNK_SIZE);
    int ccy = (int)std::floor(wy / CHUNK_SIZE);
    for (int dy = -VIEW_RADIUS; dy <= VIEW_RADIUS; ++dy)
    {
        for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; ++dx)
        {
            ensureChunk(ccx + dx, ccy + dy);
        }
    }
}

void World::unloadFarFrom(int ccx, int ccy)
{
    for (auto it = chunks_.begin(); it != chunks_.end();)
    {
        const Chunk &c = it->second;
        int dx = std::abs(c.cx - ccx);
        int dy = std::abs(c.cy - ccy);
        if (dx > VIEW_RADIUS || dy > VIEW_RADIUS)
        {
            it = chunks_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// ============================================================
// DDA- visibility-
// ============================================================

static void buildLightPolygon(const World &world, Light &L)
{
    const int N = MAX_POLY_POINTS;
    L.polyCount = N;

    for (int i = 0; i < N; ++i)
    {
        float angle = (2.0f * PI * (float)i) / (float)N;
        float dx = std::cos(angle);
        float dy = std::sin(angle);

        int cx = (int)std::floor(L.position.x);
        int cy = (int)std::floor(L.position.z);

        int stepX = (dx > 0.0f) ? 1 : ((dx < 0.0f) ? -1 : 0);
        int stepY = (dy > 0.0f) ? 1 : ((dy < 0.0f) ? -1 : 0);

        float tMaxX, tMaxY;
        if (stepX != 0)
        {
            float nextX = (stepX > 0) ? (float)(cx + 1) : (float)cx;
            tMaxX = std::abs((nextX - L.position.x) / dx);
        }
        else
        {
            tMaxX = 1e30f;
        }
        if (stepY != 0)
        {
            float nextY = (stepY > 0) ? (float)(cy + 1) : (float)cy;
            tMaxY = std::abs((nextY - L.position.z) / dy);
        }
        else
        {
            tMaxY = 1e30f;
        }

        float tDeltaX = (stepX != 0) ? std::abs(1.0f / dx) : 1e30f;
        float tDeltaY = (stepY != 0) ? std::abs(1.0f / dy) : 1e30f;

        float hitDist = L.radius;
        float t = 0.0f;

        while (t < L.radius)
        {
            if (!world.isFloor(cx, cy))
            {
                //  OVERRUN 0.05f:
                //
                hitDist = std::min(t + 0.05f, L.radius);
                break;
            }

            if (tMaxX < tMaxY)
            {
                t = tMaxX;
                cx += stepX;
                tMaxX += tDeltaX;
            }
            else
            {
                t = tMaxY;
                cy += stepY;
                tMaxY += tDeltaY;
            }
        }

        L.polyDist[i] = hitDist;
    }
}

// ============================================================
// collectLights
// ============================================================

std::vector<Light> World::collectLights(float wx, float wy, float maxDist) const
{
    std::vector<Light> result;
    result.reserve(64);

    const float maxDist2 = maxDist * maxDist;

    for (const auto &kv : chunks_)
    {
        const Chunk &c = kv.second;
        int baseX = c.cx * CHUNK_SIZE;
        int baseY = c.cy * CHUNK_SIZE;

        for (int ly = 3; ly < CHUNK_SIZE - 3; ly += 6)
        {
            for (int lx = 3; lx < CHUNK_SIZE - 3; lx += 6)
            {
                if (!c.isFloor(lx, ly))
                    continue;

                int gx = baseX + lx;
                int gy = baseY + ly;

                float px = (float)gx + 0.5f;
                float pz = (float)gy + 0.5f;

                float dx = px - wx;
                float dz = pz - wy;
                float d2 = dx * dx + dz * dz;

                if (d2 > maxDist2)
                    continue;

                Light L;
                L.position = glm::vec3(px, CEIL_HEIGHT - 0.05f, pz);
                L.color = glm::vec3(1.0f, 0.92f, 0.65f);
                L.radius = 9.0f;
                L.intensity = 1.6f;
                L.flicker = 1.0f;
                result.push_back(L);
            }
        }
    }

    std::sort(result.begin(), result.end(),
              [wx, wy](const Light &a, const Light &b)
              {
                  float ax = a.position.x - wx;
                  float az = a.position.z - wy;
                  float bx = b.position.x - wx;
                  float bz = b.position.z - wy;
                  return (ax * ax + az * az) < (bx * bx + bz * bz);
              });

    if ((int)result.size() > MAX_LIGHTS)
    {
        result.resize(MAX_LIGHTS);
    }

    for (auto &L : result)
    {
        buildLightPolygon(*this, L);
    }

    return result;
}

// ============================================================
// findSpawn
// ============================================================

bool World::findSpawn(float startX, float startY, float &outX, float &outY) const
{
    int baseX = (int)std::floor(startX);
    int baseY = (int)std::floor(startY);

    for (int r = 0; r < 64; ++r)
    {
        for (int dy = -r; dy <= r; ++dy)
        {
            for (int dx = -r; dx <= r; ++dx)
            {
                if (r > 0 && std::abs(dx) != r && std::abs(dy) != r)
                    continue;

                int cx = baseX + dx;
                int cy = baseY + dy;

                if (!isFloor(cx, cy))
                    continue;

                bool ok = true;
                for (int oy = -1; oy <= 1 && ok; ++oy)
                {
                    for (int ox = -1; ox <= 1; ++ox)
                    {
                        if (!isFloor(cx + ox, cy + oy))
                        {
                            ok = false;
                            break;
                        }
                    }
                }
                if (!ok)
                    continue;

                outX = (float)cx + 0.5f;
                outY = (float)cy + 0.5f;
                return true;
            }
        }
    }
    return false;
}