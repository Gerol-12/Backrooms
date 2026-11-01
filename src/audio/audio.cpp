#include "audio.h"
#include "core/logger.h"
#include <cstdlib>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

static ma_engine engine;
static ma_sound bgLamp;
static ma_sound stepSounds[3]; // Массив из 3 шагов (walk_01, 02, 03)
static ma_sound clickOn;
static ma_sound clickOff;

bool AudioManager::init() {
    LOG("[Audio] Initializing miniaudio engine...");
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        LOG("[Audio] ERROR: Failed to init audio engine");
        return false;
    }

    srand((unsigned int)time(NULL)); // Инициализируем рандом для шагов

    // 1. Загрузка гула ламп (зациклен)
    if (ma_sound_init_from_file(&engine, "assets/sounds/ambient/lamp.wav", MA_SOUND_FLAG_LOOPING, NULL, NULL, &bgLamp) == MA_SUCCESS) {
        ma_sound_set_volume(&bgLamp, 0.4f);
        ma_sound_start(&bgLamp);
        LOG("[Audio] Loaded and started lamp.wav");
    } else {
        LOG("[Audio] ERROR: Failed to load assets/sounds/ambient/lamp.wav");
    }

    // 2. Загрузка шагов (walk_01, 02, 03)
    const char* walkFiles[] = {
        "assets/sounds/walk/walk_01.wav",
        "assets/sounds/walk/walk_02.wav",
        "assets/sounds/walk/walk_03.wav"
    };

    for (int i = 0; i < 3; ++i) {
        if (ma_sound_init_from_file(&engine, walkFiles[i], 0, NULL, NULL, &stepSounds[i]) == MA_SUCCESS) {
            ma_sound_set_volume(&stepSounds[i], 0.6f);
            LOG("[Audio] Loaded %s", walkFiles[i]);
        } else {
            LOG("[Audio] ERROR: Failed to load %s", walkFiles[i]);
        }
    }

    // 3. Загрузка звуков фонарика (пока просто загружаем, использовать будем позже)
    if (ma_sound_init_from_file(&engine, "assets/sounds/flashlight/click_on.wav", 0, NULL, NULL, &clickOn) == MA_SUCCESS) {
        LOG("[Audio] Loaded click_on.wav");
    } else {
        LOG("[Audio] ERROR: Failed to load click_on.wav");
    }

    if (ma_sound_init_from_file(&engine, "assets/sounds/flashlight/click_off.wav", 0, NULL, NULL, &clickOff) == MA_SUCCESS) {
        LOG("[Audio] Loaded click_off.wav");
    } else {
        LOG("[Audio] ERROR: Failed to load click_off.wav");
    }

    initialized = true;
    LOG("[Audio] Initialized successfully");
    return true;
}

void AudioManager::playStep() {
    if (!initialized) return;

    // Выбираем случайный шаг от 0 до 2
    int randomIndex = rand() % 3;

    // Перематываем и играем
    ma_sound_seek_to_pcm_frame(&stepSounds[randomIndex], 0);
    ma_sound_start(&stepSounds[randomIndex]);
}

void AudioManager::playFlashlightClick(bool turnOn) {
    if (!initialized) return;
    if (turnOn) {
        ma_sound_seek_to_pcm_frame(&clickOn, 0);
        ma_sound_start(&clickOn);
    } else {
        ma_sound_seek_to_pcm_frame(&clickOff, 0);
        ma_sound_start(&clickOff);
    }
}

void AudioManager::cleanup() {
    if (!initialized) return;

    ma_sound_uninit(&bgLamp);
    for (int i = 0; i < 3; ++i) {
        ma_sound_uninit(&stepSounds[i]);
    }
    ma_sound_uninit(&clickOn);
    ma_sound_uninit(&clickOff);

    ma_engine_uninit(&engine);
    LOG("[Audio] Engine shut down");
}