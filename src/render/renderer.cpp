// src/render/renderer.cpp
#include "renderer.h"
#include "core/logger.h"
#include "render/mesh_builder.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

#define MAX_LIGHTS 12
#define MAX_POLY_POINTS 240

namespace {
    // OpenGL clip-space frustum test for an axis-aligned chunk bounding box.
    bool isAABBVisible(
        const glm::mat4 &viewProjection,
        const glm::vec3 &minPoint,
        const glm::vec3 &maxPoint)
    {
        const glm::vec4 row0(
            viewProjection[0][0], viewProjection[1][0],
            viewProjection[2][0], viewProjection[3][0]);
        const glm::vec4 row1(
            viewProjection[0][1], viewProjection[1][1],
            viewProjection[2][1], viewProjection[3][1]);
        const glm::vec4 row2(
            viewProjection[0][2], viewProjection[1][2],
            viewProjection[2][2], viewProjection[3][2]);
        const glm::vec4 row3(
            viewProjection[0][3], viewProjection[1][3],
            viewProjection[2][3], viewProjection[3][3]);

        const glm::vec4 planes[6] = {
            row3 + row0, row3 - row0,
            row3 + row1, row3 - row1,
            row3 + row2, row3 - row2
        };

        for (const glm::vec4 &plane : planes) {
            const glm::vec3 positiveVertex(
                plane.x >= 0.0f ? maxPoint.x : minPoint.x,
                plane.y >= 0.0f ? maxPoint.y : minPoint.y,
                plane.z >= 0.0f ? maxPoint.z : minPoint.z
            );

            if (glm::dot(glm::vec3(plane), positiveVertex) + plane.w < 0.0f)
                return false;
        }

        return true;
    }

    int decodeChunkCoordinate(uint32_t value) {
        return static_cast<int>(static_cast<int32_t>(value));
    }
}

bool Renderer::init(const std::string& assetsPath) {
    assetsPath_ = assetsPath;
    LOG("[Renderer] Initializing...");

    if (!shader_.loadFromFiles("shaders/basic.vert", "shaders/basic.frag")) {
        LOG("[Renderer] ERROR: Failed to load shader basic.vert/frag");
        return false;
    }

    if (!texWall_.loadFromFile(assetsPath_ + "textures/wallpaper/wallpaper_color.png")) LOG("[Renderer] ERROR: Failed to load wallpaper");
    if (!texFloor_.loadFromFile(assetsPath_ + "textures/carpet/carpet_color.png")) LOG("[Renderer] ERROR: Failed to load carpet");
    if (!texCeil_.loadFromFile(assetsPath_ + "textures/ceiling_tiles/ceiling_tiles_color.png")) LOG("[Renderer] ERROR: Failed to load ceiling");
    if (!texLamp_.loadFromFile(assetsPath_ + "textures/lamp.png")) LOG("[Renderer] ERROR: Failed to load lamp");

    if (!post_.init(1024, 768)) {
        LOG("[Renderer] ERROR: Failed to init PostProcess");
        return false;
    }

    LOG("[Renderer] Init OK");
    return true;
}

void Renderer::updateAround(const World& world, int ccx, int ccy) {
    int radius = 2;

    for (int x = ccx - radius; x <= ccx + radius; ++x) {
        for (int y = ccy - radius; y <= ccy + radius; ++y) {
            uint64_t k = World::key(x, y);
            if (meshes_.find(k) == meshes_.end()) {
                if (world.getChunk(x, y) != nullptr) {
                    buildChunkMesh(world, x, y);
                }
            }
        }
    }

    for (auto it = meshes_.begin(); it != meshes_.end(); ) {
        int cx = decodeChunkCoordinate(static_cast<uint32_t>(it->first >> 32));
        int cy = decodeChunkCoordinate(static_cast<uint32_t>(it->first & 0xFFFFFFFFu));

        if (std::abs(cx - ccx) > radius || std::abs(cy - ccy) > radius) {
            it = meshes_.erase(it);
        } else {
            ++it;
        }
    }
}

void Renderer::buildChunkMesh(const World& world, int cx, int cy) {
    MeshBuffers buf = buildMeshForChunk(world, cx, cy);
    uint64_t k = World::key(cx, cy);

    ChunkMesh& cm = meshes_[k];
    cm.floor.upload(buf.floor);
    cm.wall.upload(buf.wall);
    cm.ceil.upload(buf.ceil);
    cm.lamp.upload(buf.lamp);
}

void Renderer::resize(int w, int h) {
    glViewport(0, 0, w, h);
    post_.resize(w, h);
}

void Renderer::drawDebugLines(const glm::mat4& view, const glm::mat4& projection, int ccx, int ccy) {
    static uint32_t vao = 0, vbo = 0;
    static Shader debugShader;

    if (!debugShader.isValid()) {
        const char* vSrc = "#version 330 core\nlayout(location=0) in vec3 aPos;\nuniform mat4 uMVP;\nvoid main(){gl_Position = uMVP * vec4(aPos,1.0);}";
        const char* fSrc = "#version 330 core\nout vec4 FragColor;\nvoid main(){FragColor = vec4(1.0, 0.0, 1.0, 1.0);}"; // Розовый
        debugShader.loadFromSource(vSrc, fSrc);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    float minX = ccx * CHUNK_SIZE;
    float maxX = (ccx + 1) * CHUNK_SIZE;
    float minZ = ccy * CHUNK_SIZE;
    float maxZ = (ccy + 1) * CHUNK_SIZE;
    float y = 3.5f; // Высота стен

    float lines[] = {
        minX, 0.0f, minZ,  maxX, 0.0f, minZ, // Низ
        maxX, 0.0f, minZ,  maxX, 0.0f, maxZ,
        maxX, 0.0f, maxZ,  minX, 0.0f, maxZ,
        minX, 0.0f, maxZ,  minX, 0.0f, minZ,
        // Стойки
        minX, 0.0f, minZ,  minX, y, minZ,
        maxX, 0.0f, minZ,  maxX, y, minZ,
        maxX, 0.0f, maxZ,  maxX, y, maxZ,
        minX, 0.0f, maxZ,  minX, y, maxZ
    };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines), lines, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    debugShader.use();
    glm::mat4 mvp = projection * view;
    debugShader.setMat4("uMVP", glm::value_ptr(mvp));
    glDrawArrays(GL_LINES, 0, 16);

    glBindVertexArray(0);
}

void Renderer::render(const World& world,
                      const glm::mat4& view,
                      const glm::mat4& projection,
                      const glm::vec3& cameraPos,
                      int viewportW, int viewportH,
                      float timeSeconds,
                      bool flashlightOn,
                      const glm::vec3& flashlightDir,
                      bool showHitboxes,
                      bool showChunkBorders)
{
    post_.beginScene();

    glClearColor(0.15f, 0.14f, 0.06f, 1.0f); // Темный оливковый фон
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // --- РЕЖИМ ОТЛАДКИ (F3 + B) ---
    if (showHitboxes) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glDisable(GL_CULL_FACE); // Полностью отключаем отсечение, чтобы все стены были видны

    shader_.use();

    glm::mat4 mvp = projection * view;
    shader_.setMat4("uMVP", glm::value_ptr(mvp));
    shader_.setVec3("uCameraPos", glm::value_ptr(cameraPos));

// Туман (делаем его чуть светлее и отодвигаем дальше)
    glm::vec3 fogColor(0.2f, 0.18f, 0.1f);
    shader_.setVec3("uFogColor", glm::value_ptr(fogColor));
    shader_.setFloat("uFogStart", 5.0f);
    shader_.setFloat("uFogEnd", 25.0f); // Было 15.0, теперь туман дальше

    // Базовый свет (поднимаем, чтобы даже в тенях были видны очертания стен)
    glm::vec3 ambient(0.25f, 0.23f, 0.15f); // Было 0.15
    shader_.setVec3("uAmbient", glm::value_ptr(ambient));
    shader_.setFloat("uTime", timeSeconds);

    // Фонарик
    shader_.setInt("uFlashlightOn", flashlightOn ? 1 : 0);
    shader_.setVec3("uFlashlightDir", glm::value_ptr(flashlightDir));

    // --- СВЕТ ОТ ЛАМП ---
    std::vector<Light> lights = world.collectLights(cameraPos.x, cameraPos.z, 25.0f);
    int numLights = std::min((int)lights.size(), MAX_LIGHTS);
    shader_.setInt("uLightCount", numLights);

    if (numLights > 0) {
        std::vector<float> posData, colorData, radiusData, intensData, polyData;
        std::vector<int> polyNData;
        polyData.reserve(static_cast<size_t>(numLights) * MAX_POLY_POINTS);

        for (int i = 0; i < numLights; ++i) {
            posData.insert(posData.end(), {lights[i].position.x, lights[i].position.y, lights[i].position.z});
            colorData.insert(colorData.end(), {lights[i].color.r, lights[i].color.g, lights[i].color.b});
            radiusData.push_back(lights[i].radius);
            intensData.push_back(lights[i].intensity);
            polyNData.push_back(lights[i].polyCount);
            for (int p = 0; p < MAX_POLY_POINTS; ++p) {
                polyData.push_back(
                    p < lights[i].polyCount
                        ? lights[i].polyDist[p]
                        : lights[i].radius
                );
            }
        }

        shader_.setVec3Array("uLightPos", posData.data(), numLights);
        shader_.setVec3Array("uLightColor", colorData.data(), numLights);
        shader_.setFloatArray("uLightRadius", radiusData.data(), numLights);
        shader_.setFloatArray("uLightIntens", intensData.data(), numLights);
        shader_.setIntArray("uLightPolyN", polyNData.data(), numLights);
        shader_.setFloatArray(
            "uLightPoly",
            polyData.data(),
            numLights * MAX_POLY_POINTS
        );
    }

    // --- VISIBILITY PASS: distance + frustum culling ---
    struct VisibleChunk {
        const ChunkMesh *mesh;
    };

    std::vector<VisibleChunk> visibleChunks;
    visibleChunks.reserve(meshes_.size());

    const glm::mat4 viewProjection = projection * view;
    constexpr float MAX_RENDER_DISTANCE = CHUNK_SIZE * 3.0f;
    constexpr float MAX_RENDER_DISTANCE2 =
        MAX_RENDER_DISTANCE * MAX_RENDER_DISTANCE;

    for (const auto &pair : meshes_) {
        const int cx = decodeChunkCoordinate(
            static_cast<uint32_t>(pair.first >> 32));
        const int cz = decodeChunkCoordinate(
            static_cast<uint32_t>(pair.first & 0xFFFFFFFFu));

        const float minX = static_cast<float>(cx * CHUNK_SIZE);
        const float minZ = static_cast<float>(cz * CHUNK_SIZE);
        const glm::vec3 minPoint(minX, 0.0f, minZ);
        const glm::vec3 maxPoint(
            minX + CHUNK_SIZE,
            5.0f,
            minZ + CHUNK_SIZE
        );

        // Distance to the closest point of the chunk's XZ rectangle.
        const float closestX = std::max(minPoint.x,
            std::min(cameraPos.x, maxPoint.x));
        const float closestZ = std::max(minPoint.z,
            std::min(cameraPos.z, maxPoint.z));
        const float dx = cameraPos.x - closestX;
        const float dz = cameraPos.z - closestZ;

        if (dx * dx + dz * dz > MAX_RENDER_DISTANCE2)
            continue;

        if (!isAABBVisible(viewProjection, minPoint, maxPoint))
            continue;

        visibleChunks.push_back({ &pair.second });
    }

    // --- DRAW PASS: keep material changes grouped to minimize state changes ---
    shader_.setInt("uTex", 0);
    shader_.setFloat("uEmissive", 0.0f);

    texFloor_.bind(0);
    for (const VisibleChunk &chunk : visibleChunks)
        chunk.mesh->floor.draw();

    texWall_.bind(0);
    for (const VisibleChunk &chunk : visibleChunks)
        chunk.mesh->wall.draw();

    texCeil_.bind(0);
    for (const VisibleChunk &chunk : visibleChunks)
        chunk.mesh->ceil.draw();

    texLamp_.bind(0);
    shader_.setFloat("uEmissive", 1.0f);
    for (const VisibleChunk &chunk : visibleChunks)
        chunk.mesh->lamp.draw();

    // --- ОТЛАДКА (F3 + G) ---
    // Возвращаем заливку для линий, если был включен Wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (showChunkBorders) {
        int ccx = (int)std::floor(cameraPos.x / CHUNK_SIZE);
        int ccy = (int)std::floor(cameraPos.z / CHUNK_SIZE);
        drawDebugLines(view, projection, ccx, ccy);
    }

    post_.endSceneAndDraw(timeSeconds);
}

void Renderer::shutdown() {
    LOG("[Renderer] Shutting down...");
    meshes_.clear();
}