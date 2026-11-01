// src/render/shader.h
#pragma once

#include <string>
#include <cstdint>

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool loadFromFiles(const std::string& vertPath,
                       const std::string& fragPath);

    bool loadFromSource(const std::string& vertSrc,
                        const std::string& fragSrc);

    void use() const;

    void setMat4 (const char* name, const float* mat) const;
    void setInt  (const char* name, int value) const;
    void setFloat(const char* name, float value) const;
    void setVec3 (const char* name, const float* v) const;

    void setVec3Array (const char* name, const float* data, int count) const;
    void setFloatArray(const char* name, const float* data, int count) const;
    void setIntArray  (const char* name, const int*   data, int count) const;

    bool isValid() const { return program_ != 0; }
    uint32_t id() const { return program_; }

private:
    uint32_t program_ = 0;
    void destroy();
};