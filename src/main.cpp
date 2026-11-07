// src/main.cpp - Backrooms - VHS Found Footage
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/types.h"
#include "core/math_utils.h"
#include "core/logger.h"
#include "world/world.h"
#include "render/renderer.h"
#include "player/player.h"
#include "audio/audio.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>

constexpr int   WIN_W = 1024;
constexpr int   WIN_H = 768;
constexpr float FOV   = 75.0f;
constexpr float NEAR  = 0.05f;
constexpr float FAR   = 200.0f;
constexpr float MOUSE_SENS = 0.08f;
constexpr Uint64 TARGET_FPS = 60;

int main(int argc, char** argv) {
    Logger::init();
    LOG("=== Backrooms - VHS ===");
    unsigned int seed = (unsigned int)time(nullptr);
    if (argc > 1) seed = (unsigned int)atoi(argv[1]);
    LOG("Seed: %u", seed);

    if (!SDL_Init(SDL_INIT_VIDEO)) { LOG("SDL_Init failed: %s", SDL_GetError()); Logger::close(); return 1; }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("Backrooms", WIN_W, WIN_H, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) { LOG("SDL_CreateWindow failed"); SDL_Quit(); Logger::close(); return 1; }
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) { LOG("SDL_GL_CreateContext failed"); SDL_DestroyWindow(window); SDL_Quit(); Logger::close(); return 1; }
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) { LOG("glad failed"); SDL_GL_DestroyContext(glContext); SDL_DestroyWindow(window); SDL_Quit(); Logger::close(); return 1; }

    LOG("OpenGL Version: %s", glGetString(GL_VERSION));
    glEnable(GL_DEPTH_TEST);
    SDL_SyncWindow(window);
    SDL_SetWindowRelativeMouseMode(window, true);
    SDL_RaiseWindow(window);

    World world(seed);
    world.loadAround(16.5f, 16.5f);

    float spawnX = 16.5f, spawnY = 16.5f;
    if (world.findSpawn(16.5f, 16.5f, spawnX, spawnY)) {
        LOG("[Spawn] Safe point at (%.1f, %.1f)", spawnX, spawnY);
    } else {
        LOG("[Spawn] WARN: no safe point found");
    }

    Renderer renderer;
    if (!renderer.init("assets/")) { LOG("Renderer init failed"); return 1; }
    renderer.resize(WIN_W, WIN_H);

    Player player;
    player.camera().position().x = spawnX;
    player.camera().position().y = 1.0f; // <--- Ноги на полу
    player.camera().position().z = spawnY;

    int ccx = (int)std::floor(spawnX / CHUNK_SIZE);
    int ccy = (int)std::floor(spawnY / CHUNK_SIZE);
    renderer.updateAround(world, ccx, ccy);
    int lastCCX = ccx, lastCCY = ccy;

    bool running = true, paused = false, flashlightOn = false;
    glm::vec3 currentFlashlightDir = player.camera().front();
    bool showDebugText = false, showHitboxes = false, showChunkBorders = false;

    Uint64 lastTime = SDL_GetTicks(), startTime = lastTime;
    int frameCount = 0; Uint64 fpsTimer = lastTime;
    AudioManager audio; audio.init();
    float stepTimer = 0.0f;

    const Uint64 performanceFrequency = SDL_GetPerformanceFrequency();
    const Uint64 targetFrameTicks = performanceFrequency / TARGET_FPS;

    while (running) {
        const Uint64 frameStart = SDL_GetPerformanceCounter();
        Uint64 now = SDL_GetTicks();
        float dt = (now - lastTime) / 1000.0f; lastTime = now;
        if (dt > 0.1f) dt = 0.1f;
        float timeSec = (float)(now - startTime) / 1000.0f;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_WINDOW_RESIZED || e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                int w, h; SDL_GetWindowSizeInPixels(window, &w, &h);
                if (w > 0 && h > 0) renderer.resize(w, h);
            }
            if (e.type == SDL_EVENT_KEY_DOWN) {
                const bool* keys = SDL_GetKeyboardState(nullptr);
                bool isF3Held = keys[SDL_SCANCODE_F3];
                if (e.key.key == SDLK_ESCAPE) {
                    if (!paused) { paused = true; SDL_SetWindowRelativeMouseMode(window, false); }
                    else running = false;
                }
                else if (e.key.key == SDLK_F && !paused) { flashlightOn = !flashlightOn; audio.playFlashlightClick(flashlightOn); }
                else if (e.key.key == SDLK_F3 && !e.key.repeat) showDebugText = !showDebugText;
                else if (isF3Held && e.key.key == SDLK_B && !e.key.repeat) showHitboxes = !showHitboxes;
                else if (isF3Held && e.key.key == SDLK_G && !e.key.repeat) showChunkBorders = !showChunkBorders;
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && paused) { paused = false; SDL_SetWindowRelativeMouseMode(window, true); }
            if (e.type == SDL_EVENT_MOUSE_MOTION && !paused) player.camera().addYawPitch(e.motion.xrel * MOUSE_SENS, -e.motion.yrel * MOUSE_SENS);
        }

        if (!paused) {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            player.update(world, keys, dt);
            bool isMoving = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_D];
            bool isRunning = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
            if (isMoving && std::abs(player.camera().velocity().y) < 0.1f) {
                stepTimer += dt;
                if (stepTimer >= (isRunning ? 0.35f : 0.65f)) { audio.playStep(); stepTimer = 0.0f; }
            } else stepTimer = 0.0f;
        }

        ccx = (int)std::floor(player.camera().position().x / CHUNK_SIZE);
        ccy = (int)std::floor(player.camera().position().z / CHUNK_SIZE);
        if (ccx != lastCCX || ccy != lastCCY) {
            world.loadAround(player.camera().position().x, player.camera().position().z);
            world.unloadFarFrom(ccx, ccy);
            renderer.updateAround(world, ccx, ccy);
            lastCCX = ccx; lastCCY = ccy;
        }

        int vpW, vpH; SDL_GetWindowSizeInPixels(window, &vpW, &vpH);
        if (vpW <= 0 || vpH <= 0) { vpW = WIN_W; vpH = WIN_H; }
        glm::mat4 view = player.camera().viewMatrix();
        glm::mat4 projection = glm::perspective(deg2rad(FOV), (float)vpW / (float)vpH, NEAR, FAR);

        glm::vec3 targetDir = player.camera().front();
        glm::vec3 mixedDir = glm::mix(currentFlashlightDir, targetDir, std::min(1.0f, dt * 6.0f));
        if (glm::length(mixedDir) > 0.001f) currentFlashlightDir = glm::normalize(mixedDir);

        renderer.render(world, view, projection, player.camera().eyePosition(), vpW, vpH, timeSec, flashlightOn, currentFlashlightDir, showHitboxes, showChunkBorders);
        SDL_GL_SwapWindow(window);

        // Cap CPU/GPU work to a stable 60 FPS instead of rendering uncapped.
        const Uint64 frameElapsed = SDL_GetPerformanceCounter() - frameStart;
        if (frameElapsed < targetFrameTicks) {
            const Uint64 remainingTicks = targetFrameTicks - frameElapsed;
            const Uint32 sleepMs = static_cast<Uint32>(
                (remainingTicks * 1000ULL) / performanceFrequency);
            if (sleepMs > 0) SDL_Delay(sleepMs);
        }

        frameCount++;
        if (now - fpsTimer > 1000) {
            float fps = frameCount * 1000.0f / (now - fpsTimer);
            if (showDebugText) {
                glm::vec3 front = player.camera().front();
                float yaw = atan2(front.z, front.x) * 180.0f / PI;
                float pitch = asin(front.y) * 180.0f / PI;
                const char* facing = (yaw >= -45 && yaw < 45) ? "+X" : (yaw >= 45 && yaw < 135) ? "+Z" : (yaw >= 135 || yaw < -135) ? "-X" : "-Z";
                char title[512];
                snprintf(title, sizeof(title), "F3 | XYZ: %.2f / %.2f / %.2f | Facing: %s (Yaw: %.1f, Pitch: %.1f) | FPS: %.0f",
                    player.camera().position().x, player.camera().position().y, player.camera().position().z,
                    facing, yaw, pitch, fps);
                SDL_SetWindowTitle(window, title);
            } else {
                char title[256];
                snprintf(title, sizeof(title), "Backrooms REC | %.0f FPS | %s | pos (%.1f, %.1f) | chunk (%d, %d)%s",
                    fps, player.modeName(), player.camera().position().x, player.camera().position().z, lastCCX, lastCCY, paused ? " | PAUSED" : "");
                SDL_SetWindowTitle(window, title);
            }
            frameCount = 0; fpsTimer = now;
        }
    }

    SDL_SetWindowRelativeMouseMode(window, false);
    renderer.shutdown();
    SDL_DestroyWindow(window);
    SDL_GL_DestroyContext(glContext);
    SDL_Quit();
    LOG("=== Done ===");
    audio.cleanup();
    Logger::close();
    return 0;
}