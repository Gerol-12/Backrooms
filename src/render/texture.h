// src/render/texture.h
#pragma once

#include <string>
#include <cstdint>

class Texture {
public:
    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool loadFromFile(const std::string& path);
    void bind(int slot = 0) const;

    bool isValid() const { return id_ != 0; }
    uint32_t id() const { return id_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    uint32_t id_ = 0;
    int width_ = 0;
    int height_ = 0;

    void destroy();
};