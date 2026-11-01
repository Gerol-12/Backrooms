#pragma once

class AudioManager {
public:
    bool init();
    void playStep();
    void playFlashlightClick(bool turnOn); // Для будущего фонарика
    void cleanup();

private:
    bool initialized = false;
};