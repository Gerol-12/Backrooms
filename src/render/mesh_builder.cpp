// src/render/mesh_builder.cpp
#include "render/mesh_builder.h"
#include <glm/glm.hpp>

static void addFace(std::vector<Vertex>& out, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 n, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3) {
    out.push_back({ p0, n, uv0 });
    out.push_back({ p1, n, uv1 });
    out.push_back({ p2, n, uv2 });
    out.push_back({ p2, n, uv2 });
    out.push_back({ p3, n, uv3 });
    out.push_back({ p0, n, uv0 });
}

MeshBuffers buildMeshForChunk(const World& world, int cx, int cy) {
    MeshBuffers out;
    const Chunk* c = world.getChunk(cx, cy);
    if (!c) return out;

    int baseX = cx * CHUNK_SIZE;
    int baseZ = cy * CHUNK_SIZE;
    float uvScale = 0.5f; // 0.5 = текстура на 2 блока

    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                uint8_t type = c->at(x, y, z);
                if (type == 0) continue; // Воздух

                glm::vec3 pos(baseX + x, y, baseZ + z);
                float s = 1.0f;

                float wx = pos.x * uvScale;
                float wy = pos.y * uvScale;
                float wz = pos.z * uvScale;

                // Если это лампа (тип 2), рисуем только её нижнюю грань
                if (type == 2) {
                    if (!c->isSolid(x, y - 1, z)) {
                        addFace(out.lamp,
                            pos + glm::vec3(0, 0, s), pos + glm::vec3(s, 0, s), pos + glm::vec3(s, 0, 0), pos + glm::vec3(0, 0, 0),
                            glm::vec3(0, -1, 0),
                            {wx, wz}, {wx + s * uvScale, wz}, {wx + s * uvScale, wz + s * uvScale}, {wx, wz + s * uvScale});
                    }
                    continue;
                }

                // Обычные блоки (стены, пол, потолок)
                // 1. Верх
                if (!c->isSolid(x, y + 1, z)) {
                    if (y == 0) { // Пол
                        addFace(out.floor, pos + glm::vec3(0, s, 0), pos + glm::vec3(s, s, 0), pos + glm::vec3(s, s, s), pos + glm::vec3(0, s, s), glm::vec3(0, 1, 0), {wx, wz}, {wx + s * uvScale, wz}, {wx + s * uvScale, wz + s * uvScale}, {wx, wz + s * uvScale});
                    } else { // Верх колонны/стены
                        addFace(out.wall, pos + glm::vec3(0, s, 0), pos + glm::vec3(s, s, 0), pos + glm::vec3(s, s, s), pos + glm::vec3(0, s, s), glm::vec3(0, 1, 0), {wx, wz}, {wx + s * uvScale, wz}, {wx + s * uvScale, wz + s * uvScale}, {wx, wz + s * uvScale});
                    }
                }

                // 2. Низ
                if (!c->isSolid(x, y - 1, z)) {
                    if (y == 4) { // Потолок
                        addFace(out.ceil, pos + glm::vec3(0, 0, s), pos + glm::vec3(s, 0, s), pos + glm::vec3(s, 0, 0), pos + glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), {wx, wz}, {wx + s * uvScale, wz}, {wx + s * uvScale, wz + s * uvScale}, {wx, wz + s * uvScale});
                    } else { // Низ случайного блока (если есть)
                        addFace(out.wall, pos + glm::vec3(0, 0, s), pos + glm::vec3(s, 0, s), pos + glm::vec3(s, 0, 0), pos + glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), {wx, wz}, {wx + s * uvScale, wz}, {wx + s * uvScale, wz + s * uvScale}, {wx, wz + s * uvScale});
                    }
                }

                // 3. Перед
                if (!c->isSolid(x, y, z + 1)) {
                    addFace(out.wall, pos + glm::vec3(0, 0, s), pos + glm::vec3(0, s, s), pos + glm::vec3(s, s, s), pos + glm::vec3(s, 0, s), glm::vec3(0, 0, 1), {wx, wy}, {wx, wy + s * uvScale}, {wx + s * uvScale, wy + s * uvScale}, {wx + s * uvScale, wy});
                }

                // 4. Зад
                if (!c->isSolid(x, y, z - 1)) {
                    addFace(out.wall, pos + glm::vec3(s, 0, 0), pos + glm::vec3(s, s, 0), pos + glm::vec3(0, s, 0), pos + glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), {wx, wy}, {wx, wy + s * uvScale}, {wx + s * uvScale, wy + s * uvScale}, {wx + s * uvScale, wy});
                }

                // 5. Право
                if (!c->isSolid(x + 1, y, z)) {
                    addFace(out.wall, pos + glm::vec3(s, 0, s), pos + glm::vec3(s, s, s), pos + glm::vec3(s, s, 0), pos + glm::vec3(s, 0, 0), glm::vec3(1, 0, 0), {wz, wy}, {wz, wy + s * uvScale}, {wz + s * uvScale, wy + s * uvScale}, {wz + s * uvScale, wy});
                }

                // 6. Лево
                if (!c->isSolid(x - 1, y, z)) {
                    addFace(out.wall, pos + glm::vec3(0, 0, 0), pos + glm::vec3(0, s, 0), pos + glm::vec3(0, s, s), pos + glm::vec3(0, 0, s), glm::vec3(-1, 0, 0), {wz, wy}, {wz, wy + s * uvScale}, {wz + s * uvScale, wy + s * uvScale}, {wz + s * uvScale, wy});
                }
            }
        }
    }
    return out;
}