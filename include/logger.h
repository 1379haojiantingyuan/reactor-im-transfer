#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>

enum LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

inline void log_message(LogLevel level, const std::string& message) {
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    std::cout << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] ";

    switch (level) {
        case INFO:    std::cout << "[INFO] "; break;
        case WARNING: std::cout << "[WARNING] "; break;
        case ERROR:   std::cerr << "[ERROR] "; break;
        case DEBUG:   std::cout << "[DEBUG] "; break;
    }

    if (level == ERROR) {
        std::cerr << message << std::endl;
    } else {
        std::cout << message << std::endl;
    }
}

#define LOG_INFO(msg) log_message(INFO, msg)
#define LOG_ERROR(msg) log_message(ERROR, msg)

#endif // LOGGER_H
