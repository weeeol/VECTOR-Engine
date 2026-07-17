#include "Engine/Core/Logger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace VECTOR {

    void Logger::Init() {
        VECTOR_LOG_INFO("Logger Initialized!");
    }

    void Logger::Log(LogLevel level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        std::tm timeinfo;
        localtime_s(&timeinfo, &in_time_t);
        ss << std::put_time(&timeinfo, "%Y-%m-%d %X");

        std::string levelStr;
        switch (level) {
            case LogLevel::INFO:  levelStr = "INFO"; break;
            case LogLevel::WARN:  levelStr = "WARN"; break;
            case LogLevel::ERR: levelStr = "ERROR"; break;
        }

        std::cout << "[" << ss.str() << "] [VECTOR] [" << levelStr << "]: " << message << std::endl;
    }

} // namespace VECTOR
