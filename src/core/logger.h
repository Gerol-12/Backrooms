#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <cstdarg>

class Logger {
public:
    static void init() {
        // Открываем файл log.txt в корне проекта (рядом с .exe)
        file.open("log.txt", std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            log("=== BACKROOMS LOG STARTED ===");
        }
    }

    static void close() {
        if (file.is_open()) {
            log("=== BACKROOMS LOG ENDED ===");
            file.close();
        }
    }

    // Функция для записи в лог (работает как printf)
    static void log(const char* format, ...) {
        // 1. Получаем текущее время
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm parts;
    #ifdef _WIN32
        localtime_s(&parts, &now_c);
    #else
        localtime_r(&now_c, &parts);
    #endif

        char timeStr[16];
        strftime(timeStr, sizeof(timeStr), "[%H:%M:%S] ", &parts);

        // 2. Форматируем сообщение
        char buffer[2048];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        // 3. Выводим в консоль
        std::cout << timeStr << buffer << "\n";

        // 4. Пишем в файл
        if (file.is_open()) {
            file << timeStr << buffer << "\n";
            file.flush(); // Сбрасываем буфер, чтобы не потерять данные при вылете
        }
    }

private:
    static inline std::ofstream file;
};

// Макрос, чтобы было удобно писать (короткое имя)
#define LOG(...) Logger::log(__VA_ARGS__)