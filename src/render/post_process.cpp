// src/render/post_process.cpp

#include "render/post_process.h"

#include <glad/glad.h>
#include <cstdio>

// ============================================================
//  Fullscreen quad (2 треугольника)
// ============================================================

static const float QUAD_VERTS[] = {
    // x,  y,    u,    v
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,

    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f,  1.0f, 0.0f, 1.0f
};

PostProcess::~PostProcess() {
    shutdown();
}

bool PostProcess::init(int width, int height) {
    width_  = width;
    height_ = height;

    if (!vhsShader_.loadFromFiles("shaders/vhs.vert", "shaders/vhs.frag")) {
        printf("[PostProcess] VHS shader failed\n");
        return false;
    }

    glGenVertexArrays(1, &quadVao_);
    glGenBuffers(1, &quadVbo_);
    glBindVertexArray(quadVao_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTS), QUAD_VERTS, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    createFbo();
    printf("[PostProcess] Init OK (%dx%d)\n", width, height);
    return true;
}

void PostProcess::createFbo() {
    destroyFbo();

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &colorTex_);
    glBindTexture(GL_TEXTURE_2D, colorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTex_, 0);

    glGenRenderbuffers(1, &depthRbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo_);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("[PostProcess] FBO incomplete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PostProcess::destroyFbo() {
    if (fbo_)      { glDeleteFramebuffers(1, &fbo_);       fbo_ = 0; }
    if (colorTex_) { glDeleteTextures(1, &colorTex_);      colorTex_ = 0; }
    if (depthRbo_) { glDeleteRenderbuffers(1, &depthRbo_); depthRbo_ = 0; }
}

void PostProcess::resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == width_ && height == height_) return;
    width_  = width;
    height_ = height;
    createFbo();
}

void PostProcess::beginScene() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
}

void PostProcess::endSceneAndDraw(float time) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width_, height_);

    glDisable(GL_DEPTH_TEST);

    vhsShader_.use();
    vhsShader_.setInt("uScene", 0);
    vhsShader_.setFloat("uTime", time);
    vhsShader_.setFloat("uResolutionX", (float)width_);
    vhsShader_.setFloat("uResolutionY", (float)height_);
    vhsShader_.setInt("uVhsQuality", quality_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTex_);

    glBindVertexArray(quadVao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

void PostProcess::shutdown() {
    destroyFbo();
    if (quadVbo_) { glDeleteBuffers(1, &quadVbo_);      quadVbo_ = 0; }
    if (quadVao_) { glDeleteVertexArrays(1, &quadVao_); quadVao_ = 0; }
}