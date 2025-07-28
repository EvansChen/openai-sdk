#include "logger.h"
#include <iostream>
#include <mutex>

namespace openai_agents {

std::shared_ptr<Logger> Logger::instance_;
std::shared_ptr<Logger> logger = Logger::get_instance();

std::shared_ptr<Logger> Logger::get_instance() {
    static std::once_flag flag;
    std::call_once(flag, []() {
        instance_ = std::shared_ptr<Logger>(new Logger());
    });
    return instance_;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (static_cast<int>(level) >= static_cast<int>(current_level_)) {
        std::string level_str;
        switch (level) {
            case LogLevel::DEBUG: level_str = "DEBUG"; break;
            case LogLevel::INFO: level_str = "INFO"; break;
            case LogLevel::WARNING: level_str = "WARNING"; break;
            case LogLevel::ERROR: level_str = "ERROR"; break;
        }
        std::cout << "[" << level_str << "] " << message << std::endl;
    }
}

void Logger::set_level(LogLevel level) {
    current_level_ = level;
}

} // namespace openai_agents