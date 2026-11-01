// src/render/mesh.cpp

#include "render/mesh.h"

#include <glad/glad.h>
#include <cstddef>
#include <utility>

Mesh::~Mesh() { destroy(); }

Mesh::Mesh(Mesh&& other) noexcept {
    vao_ = other.vao_;
    vbo_ = other.vbo_;
    vertexCount_ = other.vertexCount_;
    other.vao_ = 0;
    other.vbo_ = 0;
    other.vertexCount_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        destroy();
        vao_ = other.vao_;
        vbo_ = other.vbo_;
        vertexCount_ = other.vertexCount_;
        other.vao_ = 0;
        other.vbo_ = 0;
        other.vertexCount_ = 0;
    }
    return *this;
}

void Mesh::destroy() {
    if (vao_ != 0) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_);      vbo_ = 0; }
    vertexCount_ = 0;
}

void Mesh::upload(const std::vector<Vertex>& verts) {
    destroy();
    if (verts.empty()) return;

    vertexCount_ = (int)verts.size();

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(Vertex),
                 verts.data(),
                 GL_STATIC_DRAW);

    // Position — location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Normal — location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // UV — location 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (vertexCount_ == 0) return;
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
}