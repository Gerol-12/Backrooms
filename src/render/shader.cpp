#include "render/shader.h"
#include "core/logger.h"
#include <glad/glad.h>
#include <vector>

Shader::~Shader() {
    destroy();
}

bool Shader::loadFromFiles(const std::string& vertPath, const std::string& fragPath) {
    // Читаем файлы через стандартную библиотеку (упрощенно)
    FILE* f = fopen(vertPath.c_str(), "rb");
    if (!f) { LOG("[Shader] Failed to open %s", vertPath.c_str()); return false; }
    fseek(f, 0, SEEK_END); long vSize = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<char> vSrc(vSize + 1); fread(vSrc.data(), 1, vSize, f); fclose(f); vSrc[vSize] = 0;

    f = fopen(fragPath.c_str(), "rb");
    if (!f) { LOG("[Shader] Failed to open %s", fragPath.c_str()); return false; }
    fseek(f, 0, SEEK_END); long fSize = ftell(f); fseek(f, 0, SEEK_SET);
    std::vector<char> fSrc(fSize + 1); fread(fSrc.data(), 1, fSize, f); fclose(f); fSrc[fSize] = 0;

    return loadFromSource(vSrc.data(), fSrc.data());
}

bool Shader::loadFromSource(const std::string& vertSrc, const std::string& fragSrc) {
    uint32_t vs = glCreateShader(GL_VERTEX_SHADER);
    const char* vCode = vertSrc.c_str();
    glShaderSource(vs, 1, &vCode, NULL);
    glCompileShader(vs);

    int success;
    char infoLog[1024];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 1024, NULL, infoLog);
        LOG("[Shader] ERROR compiling VERTEX shader:\n%s", infoLog);
        return false;
    }

    uint32_t fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fCode = fragSrc.c_str();
    glShaderSource(fs, 1, &fCode, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 1024, NULL, infoLog);
        LOG("[Shader] ERROR compiling FRAGMENT shader:\n%s", infoLog);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);

    glGetProgramiv(program_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program_, 1024, NULL, infoLog);
        LOG("[Shader] ERROR linking program:\n%s", infoLog);
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    LOG("[Shader] Successfully compiled and linked."); // Добавим лог об успехе
    return true;
}

void Shader::use() const {
    glUseProgram(program_);
}

void Shader::setMat4(const char* name, const float* mat) const {
    glUniformMatrix4fv(glGetUniformLocation(program_, name), 1, GL_FALSE, mat);
}

void Shader::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(program_, name), value);
}

void Shader::setFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(program_, name), value);
}

void Shader::setVec3(const char* name, const float* v) const {
    glUniform3fv(glGetUniformLocation(program_, name), 1, v);
}

void Shader::setVec3Array(const char* name, const float* data, int count) const {
    glUniform3fv(glGetUniformLocation(program_, name), count, data);
}

void Shader::setFloatArray(const char* name, const float* data, int count) const {
    glUniform1fv(glGetUniformLocation(program_, name), count, data);
}

void Shader::setIntArray(const char* name, const int* data, int count) const {
    glUniform1iv(glGetUniformLocation(program_, name), count, data);
}

void Shader::destroy() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}