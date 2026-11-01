// src/render/mesh_builder.cpp
#include "render/mesh_builder.h"
#include <glm/glm.hpp>

// Новая функция для отрисовки грани с правильными UV-координатами
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

    // Масштаб текстур. 0.5 = текстура на 2 блока. 0.25 = на 4 блока.
    float uvScale = 0.5f;

    for (int y = 0; y < CHUNK_HEIGHT; ++y) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                if (!c->isSolid(x, y, z)) continue;

                glm::vec3 pos(baseX + x, y, baseZ + z);
                float s = 1.0f;

                // Мировые координаты для UV (чтобы текстура не прыгала от блока к блоку)
                float wx = pos.x * uvScale;
                float wy = pos.y * uvScale;
                float wz = pos.z * uvScale;

                // 1. Верхняя грань (Y + 1)
                if (!c->isSolid(x, y + 1, z)) {
                    addFace(out.wall,
                        pos + glm::vec3(0, s, 0), pos + glm::vec3(s, s, 0), pos + glm::vec3(s, s, s), pos + glm::vec3(0, s, s),
                        glm::vec3(0, 1, 0),
                        {wx, wz}, {wx + s * uvScale, wz}, {wx + s * uvScale, wz + s * uvScale}, {wx, wz + s * uvScale});
                }

                // 2. Нижняя грань (Y - 1)
                if (!c->isSolid(x, y - 1, z)) {
                    addFace(out.wall,
                        pos + glm::vec3(0, 0, s), pos + glm::vec3(s, 0, s), pos + glm::vec3(s, 0, 0), pos + glm::vec3(0, 0, 0),
                        glm::vec3(0, -1, 0),
                        {wx, wz}, {wx + s * uvScale, wz}, {wx + s * uvScale, wz + s * uvScale}, {wx, wz + s * uvScale});
                }

                // 3. Передняя грань (Z + 1)
                if (!c->isSolid(x, y, z + 1)) {
                    addFace(out.wall,
                        pos + glm::vec3(0, 0, s), pos + glm::vec3(0, s, s), pos + glm::vec3(s, s, s), pos + glm::vec3(s, 0, s),
                        glm::vec3(0, 0, 1),
                        {wx, wy}, {wx, wy + s * uvScale}, {wx + s * uvScale, wy + s * uvScale}, {wx + s * uvScale, wy});
                }

                // 4. Задняя грань (Z - 1)
                if (!c->isSolid(x, y, z - 1)) {
                    addFace(out.wall,
                        pos + glm::vec3(s, 0, 0), pos + glm::vec3(s, s, 0), pos + glm::vec3(0, s, 0), pos + glm::vec3(0, 0, 0),
                        glm::vec3(0, 0, -1),
                        {wx, wy}, {wx, wy + s * uvScale}, {wx + s * uvScale, wy + s * uvScale}, {wx + s * uvScale, wy});
                }

                // 5. Правая грань (X + 1)
                if (!c->isSolid(x + 1, y, z)) {
                    addFace(out.wall,
                        pos + glm::vec3(s, 0, s), pos + glm::vec3(s, s, s), pos + glm::vec3(s, s, 0), pos + glm::vec3(s, 0, 0),
                        glm::vec3(1, 0, 0),
                        {wz, wy}, {wz, wy + s * uvScale}, {wz + s * uvScale, wy + s * uvScale}, {wz + s * uvScale, wy});
                }

                // 6. Левая грань (X - 1)
                if (!c->isSolid(x - 1, y, z)) {
                    addFace(out.wall,
                        pos + glm::vec3(0, 0, 0), pos + glm::vec3(0, s, 0), pos + glm::vec3(0, s, s), pos + glm::vec3(0, 0, s),
                        glm::vec3(-1, 0, 0),
                        {wz, wy}, {wz, wy + s * uvScale}, {wz + s * uvScale, wy + s * uvScale}, {wz + s * uvScale, wy});
                }
            }
        }
    }
    return out;
}