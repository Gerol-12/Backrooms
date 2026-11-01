// src/render/mesh_builder.cpp

#include "render/mesh_builder.h"
#include <glm/glm.hpp>

// ============================================================
//  Примитивы
// ============================================================

static void addWallQuad(std::vector<Vertex>& out, const glm::vec3& p0, const glm::vec3& p1,
                        const glm::vec3& normal, float uStart, float uEnd, float vStart, float vEnd) {
    glm::vec3 bl = p0;
    glm::vec3 br = p1;
    glm::vec3 tl(bl.x, bl.y + CEIL_HEIGHT, bl.z);
    glm::vec3 tr(br.x, br.y + CEIL_HEIGHT, br.z);

    // Проверяем, куда смотрят треугольники относительно нормали
    glm::vec3 edge1 = br - bl;
    glm::vec3 edge2 = tl - bl;
    glm::vec3 cross = glm::cross(edge1, edge2);

    if (glm::dot(cross, normal) < 0.0f) {
        // Если порядок неверный, разворачиваем треугольники (против часовой)
        out.push_back({ bl, normal, { uStart, vStart } });
        out.push_back({ tr, normal, { uEnd, vEnd } });
        out.push_back({ br, normal, { uEnd, vStart } });

        out.push_back({ tr, normal, { uEnd, vEnd } });
        out.push_back({ tl, normal, { uStart, vEnd } });
        out.push_back({ bl, normal, { uStart, vStart } });
    } else {
        // Если порядок верный, оставляем как было
        out.push_back({ bl, normal, { uStart, vStart } });
        out.push_back({ br, normal, { uEnd, vStart } });
        out.push_back({ tr, normal, { uEnd, vEnd } });

        out.push_back({ tr, normal, { uEnd, vEnd } });
        out.push_back({ tl, normal, { uStart, vEnd } });
        out.push_back({ bl, normal, { uStart, vStart } });
    }
}
static void addHorizontalQuad(std::vector<Vertex>& out,
                              float x0, float x1,
                              float z0, float z1,
                              float y,
                              float tile,
                              const glm::vec3& normal)
{
    float u0 = x0 / tile;
    float u1 = x1 / tile;
    float v0 = z0 / tile;
    float v1 = z1 / tile;

    out.push_back({ { x0, y, z0 }, normal, { u0, v0 } });
    out.push_back({ { x1, y, z0 }, normal, { u1, v0 } });
    out.push_back({ { x1, y, z1 }, normal, { u1, v1 } });

    out.push_back({ { x1, y, z1 }, normal, { u1, v1 } });
    out.push_back({ { x0, y, z1 }, normal, { u0, v1 } });
    out.push_back({ { x0, y, z0 }, normal, { u0, v0 } });
}

// ============================================================
//  Генерация
// ============================================================

MeshBuffers buildMeshForChunk(const World& world, int cx, int cy) {
    MeshBuffers buf;

    int baseX = cx * CHUNK_SIZE;
    int baseY = cy * CHUNK_SIZE;

    for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
        for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
            int wx = baseX + lx;
            int wy = baseY + ly;

            float x0 = (float)wx;
            float x1 = x0 + 1.0f;
            float z0 = (float)wy;
            float z1 = z0 + 1.0f;

            if (world.isFloor(wx, wy)) {
                addHorizontalQuad(buf.floor, x0, x1, z0, z1,
                                  0.0f, FLOOR_TILE,
                                  glm::vec3(0, 1, 0));
                addHorizontalQuad(buf.ceil, x0, x1, z0, z1,
                                  CEIL_HEIGHT, CEIL_TILE,
                                  glm::vec3(0, -1, 0));
            } else {
                if (world.isFloor(wx, wy - 1)) {
                    addWallQuad(buf.wall,
                        glm::vec3(x0, 0.0f, z0),
                        glm::vec3(x1, 0.0f, z0),
                        glm::vec3(0, 0, 1),
                        x0 / WALL_TILE, x1 / WALL_TILE,
                        0.0f, CEIL_HEIGHT / WALL_TILE);
                }
                if (world.isFloor(wx, wy + 1)) {
                    addWallQuad(buf.wall,
                        glm::vec3(x1, 0.0f, z1),
                        glm::vec3(x0, 0.0f, z1),
                        glm::vec3(0, 0, -1),
                        x1 / WALL_TILE, x0 / WALL_TILE,
                        0.0f, CEIL_HEIGHT / WALL_TILE);
                }
                if (world.isFloor(wx - 1, wy)) {
                    addWallQuad(buf.wall,
                        glm::vec3(x0, 0.0f, z1),
                        glm::vec3(x0, 0.0f, z0),
                        glm::vec3(1, 0, 0),
                        z1 / WALL_TILE, z0 / WALL_TILE,
                        0.0f, CEIL_HEIGHT / WALL_TILE);
                }
                if (world.isFloor(wx + 1, wy)) {
                    addWallQuad(buf.wall,
                        glm::vec3(x1, 0.0f, z0),
                        glm::vec3(x1, 0.0f, z1),
                        glm::vec3(-1, 0, 0),
                        z0 / WALL_TILE, z1 / WALL_TILE,
                        0.0f, CEIL_HEIGHT / WALL_TILE);
                }
            }
        }
    }

    // ---- Лампы ----
    const float lampY = CEIL_HEIGHT - 0.02f;
    const float lampHalf = 0.55f;

    for (int ly = 3; ly < CHUNK_SIZE - 3; ly += 6) {
        for (int lx = 3; lx < CHUNK_SIZE - 3; lx += 6) {
            int wx = baseX + lx;
            int wy = baseY + ly;

            if (!world.isFloor(wx, wy)) continue;

            float cxp = (float)wx + 0.5f;
            float czp = (float)wy + 0.5f;

            glm::vec3 n(0.0f, -1.0f, 0.0f);

            glm::vec3 v0(cxp - lampHalf, lampY, czp - lampHalf);
            glm::vec3 v1(cxp + lampHalf, lampY, czp - lampHalf);
            glm::vec3 v2(cxp + lampHalf, lampY, czp + lampHalf);
            glm::vec3 v3(cxp - lampHalf, lampY, czp + lampHalf);

            buf.lamp.push_back({ v0, n, { 0, 0 } });
            buf.lamp.push_back({ v1, n, { 1, 0 } });
            buf.lamp.push_back({ v2, n, { 1, 1 } });

            buf.lamp.push_back({ v2, n, { 1, 1 } });
            buf.lamp.push_back({ v3, n, { 0, 1 } });
            buf.lamp.push_back({ v0, n, { 0, 0 } });
        }
    }

    return buf;
}