#pragma once

#include <string>
#include <iostream>

namespace VECTOR {

    enum class LogLevel {
        INFO,
        WARN,
        ERR
    };

    class Logger {
    public:
        static void Init();
        static void Log(LogLevel level, const std::string& message);
    };

} // namespace VECTOR

// Macros for easy usage
#define VECTOR_LOG_INFO(message)  VECTOR::Logger::Log(VECTOR::LogLevel::INFO, message)
#define VECTOR_LOG_WARN(message)  VECTOR::Logger::Log(VECTOR::LogLevel::WARN, message)
#define VECTOR_LOG_ERROR(message) VECTOR::Logger::Log(VECTOR::LogLevel::ERR, message)
