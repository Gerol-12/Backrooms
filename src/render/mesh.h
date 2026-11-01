// src/render/mesh.h
#pragma once

#include <cstdint>
#include <vector>
#include "core/types.h"

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<Vertex>& verts);
    void draw() const;
    void destroy();

    bool isValid() const { return vao_ != 0 && vertexCount_ > 0; }
    int vertexCount() const { return vertexCount_; }

private:
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    int vertexCount_ = 0;
};

// ============================================================
//  ChunkMesh
// ============================================================

struct ChunkMesh {
    Mesh floor;
    Mesh wall;
    Mesh ceil;
    Mesh lamp;

    bool isEmpty() const {
        return !floor.isValid() && !wall.isValid()
            && !ceil.isValid()  && !lamp.isValid();
    }
};