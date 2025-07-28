#pragma once

/**
 * Logging utilities for agents
 */

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <map>
#include <any>

namespace openai_agents {

// Log levels
enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
};

// Log entry
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string message;
    std::string logger_name;
    std::map<std::string, std::any> metadata;
    std::string file;
    int line;
    std::string function;
};

// Forward declarations
class LogHandler;

// Logger interface
class Logger {
private:
    std::string name_;
    LogLevel min_level_;
    std::vector<std::shared_ptr<LogHandler>> handlers_;

public:
    Logger(const std::string& name, LogLevel min_level = LogLevel::Info);

    // Logging methods
    void debug(const std::string& message, const std::map<std::string, std::any>& metadata = {});
    void info(const std::string& message, const std::map<std::string, std::any>& metadata = {});
    void warning(const std::string& message, const std::map<std::string, std::any>& metadata = {});
    void error(const std::string& message, const std::map<std::string, std::any>& metadata = {});
    void critical(const std::string& message, const std::map<std::string, std::any>& metadata = {});

    void log(LogLevel level, const std::string& message, const std::map<std::string, std::any>& metadata = {},
             const std::string& file = "", int line = 0, const std::string& function = "");

    // Handler management
    void add_handler(std::shared_ptr<LogHandler> handler);
    void remove_handler(std::shared_ptr<LogHandler> handler);
    void clear_handlers();

    // Configuration
    void set_level(LogLevel level) { min_level_ = level; }
    LogLevel get_level() const { return min_level_; }
    const std::string& get_name() const { return name_; }
    bool is_enabled_for(LogLevel level) const { return level >= min_level_; }
};

// Log handler base class
class LogHandler {
public:
    virtual ~LogHandler() = default;
    virtual void handle(const LogEntry& entry) = 0;
    virtual void flush() {}
};

// Console handler
class ConsoleLogHandler : public LogHandler {
private:
    bool use_colors_;
    LogLevel min_level_;

public:
    ConsoleLogHandler(bool use_colors = true, LogLevel min_level = LogLevel::Info);
    void handle(const LogEntry& entry) override;

private:
    std::string format_entry(const LogEntry& entry) const;
    std::string level_to_string(LogLevel level) const;
    std::string level_to_color(LogLevel level) const;
};

// File handler
class FileLogHandler : public LogHandler {
private:
    std::string filename_;
    LogLevel min_level_;
    size_t max_file_size_;
    size_t max_files_;
    mutable std::ofstream file_;

public:
    FileLogHandler(const std::string& filename, LogLevel min_level = LogLevel::Info,
                   size_t max_file_size = 10 * 1024 * 1024, size_t max_files = 5);
    ~FileLogHandler();

    void handle(const LogEntry& entry) override;
    void flush() override;

private:
    void rotate_files();
    std::string format_entry(const LogEntry& entry) const;
};

// Memory handler (for testing/debugging)
class MemoryLogHandler : public LogHandler {
private:
    mutable std::vector<LogEntry> entries_;
    size_t max_entries_;

public:
    MemoryLogHandler(size_t max_entries = 1000);
    void handle(const LogEntry& entry) override;
    
    std::vector<LogEntry> get_entries() const { return entries_; }
    void clear() { entries_.clear(); }
};

// Custom handler using lambda
class LambdaLogHandler : public LogHandler {
private:
    std::function<void(const LogEntry&)> handler_func_;

public:
    LambdaLogHandler(std::function<void(const LogEntry&)> handler_func);
    void handle(const LogEntry& entry) override;
};

// Logger manager
class LoggerManager {
private:
    std::map<std::string, std::shared_ptr<Logger>> loggers_;
    LogLevel default_level_;
    std::vector<std::shared_ptr<LogHandler>> default_handlers_;

public:
    LoggerManager(LogLevel default_level = LogLevel::Info);

    // Logger creation and retrieval
    std::shared_ptr<Logger> get_logger(const std::string& name);
    std::shared_ptr<Logger> create_logger(const std::string& name, LogLevel level = LogLevel::Info);

    // Default handlers
    void add_default_handler(std::shared_ptr<LogHandler> handler);
    void clear_default_handlers();

    // Global configuration
    void set_default_level(LogLevel level) { default_level_ = level; }
    LogLevel get_default_level() const { return default_level_; }

    // Utilities
    void configure_console_logging(bool use_colors = true, LogLevel level = LogLevel::Info);
    void configure_file_logging(const std::string& filename, LogLevel level = LogLevel::Info);
    void disable_logging();
};

// Global logger manager
LoggerManager& get_global_logger_manager();

// Convenience functions
std::shared_ptr<Logger> get_logger(const std::string& name);
void configure_logging(LogLevel level = LogLevel::Info, bool console = true, const std::string& file = "");

// Macros for easy logging with file/line info
#define LOG_DEBUG(logger, message) (logger)->log(LogLevel::Debug, message, {}, __FILE__, __LINE__, __func__)
#define LOG_INFO(logger, message) (logger)->log(LogLevel::Info, message, {}, __FILE__, __LINE__, __func__)
#define LOG_WARNING(logger, message) (logger)->log(LogLevel::Warning, message, {}, __FILE__, __LINE__, __func__)
#define LOG_ERROR(logger, message) (logger)->log(LogLevel::Error, message, {}, __FILE__, __LINE__, __func__)
#define LOG_CRITICAL(logger, message) (logger)->log(LogLevel::Critical, message, {}, __FILE__, __LINE__, __func__)

} // namespace openai_agents