// src/render/post_process.h
#pragma once

#include <glm/glm.hpp>
#include "render/shader.h"

// ============================================================
//  PostProcess — рендерит сцену в FBO и накладывает VHS-эффект
// ============================================================

class PostProcess {
public:
    PostProcess() = default;
    ~PostProcess();

    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    bool init(int width, int height);

    void resize(int width, int height);

    void beginScene();

    void endSceneAndDraw(float time);
    void setQuality(int quality) { quality_ = quality < 0 ? 0 : (quality > 2 ? 2 : quality); }

    void shutdown();

private:
    uint32_t fbo_       = 0;
    uint32_t colorTex_  = 0;
    uint32_t depthRbo_  = 0;
    uint32_t quadVao_   = 0;
    uint32_t quadVbo_   = 0;

    Shader   vhsShader_;
    int      width_  = 0;
    int      height_ = 0;
    int      quality_ = 2;

    void createFbo();
    void destroyFbo();
};