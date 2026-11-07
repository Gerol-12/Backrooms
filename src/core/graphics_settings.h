#pragma once

#include <algorithm>
#include <fstream>
#include <string>

struct GraphicsSettings {
    int maxFps = 60;
    bool vsync = true;
    int viewDistance = 3;
    int lightingQuality = 2; // 0=low, 1=medium, 2=high
    int vhsQuality = 2;      // 0=off, 1=low, 2=full

    static GraphicsSettings load(const std::string& path = "graphics.ini") {
        GraphicsSettings settings;
        std::ifstream file(path);
        if (!file.is_open()) {
            settings.save(path);
            return settings;
        }

        std::string line;
        while (std::getline(file, line)) {
            const auto separator = line.find('=');
            if (separator == std::string::npos) continue;
            const std::string key = line.substr(0, separator);
            const std::string value = line.substr(separator + 1);
            try {
                if (key == "max_fps") settings.maxFps = std::stoi(value);
                else if (key == "vsync") settings.vsync = (std::stoi(value) != 0);
                else if (key == "view_distance") settings.viewDistance = std::stoi(value);
                else if (key == "lighting_quality") settings.lightingQuality = std::stoi(value);
                else if (key == "vhs_quality") settings.vhsQuality = std::stoi(value);
            } catch (...) {
                // Ignore malformed values and keep safe defaults.
            }
        }

        settings.maxFps = (settings.maxFps == 30) ? 30 : 60;
        settings.viewDistance = std::clamp(settings.viewDistance, 2, 6);
        settings.lightingQuality = std::clamp(settings.lightingQuality, 0, 2);
        settings.vhsQuality = std::clamp(settings.vhsQuality, 0, 2);
        return settings;
    }

    bool save(const std::string& path = "graphics.ini") const {
        std::ofstream file(path, std::ios::trunc);
        if (!file.is_open()) return false;
        file << "# Backrooms graphics settings\n"
             << "max_fps=" << ((maxFps == 30) ? 30 : 60) << '\n'
             << "vsync=" << (vsync ? 1 : 0) << '\n'
             << "view_distance=" << std::clamp(viewDistance, 2, 6) << '\n'
             << "lighting_quality=" << std::clamp(lightingQuality, 0, 2) << '\n'
             << "vhs_quality=" << std::clamp(vhsQuality, 0, 2) << '\n';
        return file.good();
    }
};
