// src/render/mesh_builder.h
#pragma once

#include <vector>
#include "core/types.h"
#include "world/chunk.h"
#include "world/world.h"

struct MeshBuffers {
    std::vector<Vertex> floor;
    std::vector<Vertex> wall;
    std::vector<Vertex> ceil;
    std::vector<Vertex> lamp;
};

MeshBuffers buildMeshForChunk(const World& world, int cx, int cy);