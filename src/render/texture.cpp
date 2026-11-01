// src/render/texture.cpp

#include "render/texture.h"

#include <glad/glad.h>
#include "lodepng.h"

#include <cstdio>
#include <vector>

Texture::~Texture() { destroy(); }

void Texture::destroy() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        id_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

bool Texture::loadFromFile(const std::string& path) {
    destroy();

    std::vector<unsigned char> pixels;
    unsigned int w = 0, h = 0;
    unsigned int err = lodepng::decode(pixels, w, h, path);
    if (err) {
        printf("[Texture] Failed to load %s: %s\n",
               path.c_str(), lodepng_error_text(err));
        return false;
    }

    width_ = (int)w;
    height_ = (int)h;

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 (GLsizei)w, (GLsizei)h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE,
                 pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    printf("[Texture] Loaded %s (%ux%u)\n", path.c_str(), w, h);
    return true;
}

void Texture::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, id_);
}