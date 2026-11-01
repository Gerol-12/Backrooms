// src/render/renderer.h
#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

#include "render/shader.h"
#include "render/texture.h"
#include "render/mesh.h"
#include "render/post_process.h"
#include "world/world.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool init(const std::string& assetsPath);

    void updateAround(const World& world, int ccx, int ccy);

    void render(const World& world,
                const glm::mat4& view,
                const glm::mat4& projection,
                const glm::vec3& cameraPos,
                int viewportW, int viewportH,
                float timeSeconds,
                bool flashlightOn,
                const glm::vec3& flashlightDir,
                bool showHitboxes,
                bool showChunkBorders);

    void resize(int w, int h);
    void shutdown();

    size_t meshCount() const { return meshes_.size(); }

private:
    Shader  shader_;
    Texture texWall_;
    Texture texFloor_;
    Texture texCeil_;
    Texture texLamp_;

    PostProcess post_;

    std::unordered_map<uint64_t, ChunkMesh> meshes_;

    std::string assetsPath_;

    void buildChunkMesh(const World& world, int cx, int cy);
    void drawDebugLines(const glm::mat4& view, const glm::mat4& projection, int ccx, int ccy);
};