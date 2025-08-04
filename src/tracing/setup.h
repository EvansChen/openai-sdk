#pragma once

/**
 * Tracing Setup and Configuration for OpenAI Agents Framework
 * 
 * This module provides setup and configuration utilities for the tracing system.
 */

#include "processor_interface.h"
#include "processors.h"
#include <memory>
#include <string>
#include <vector>

namespace openai_agents {
namespace tracing {

/**
 * Tracing configuration structure
 */
struct TracingConfig {
    bool enabled = true;
    std::string default_processor_type = "console";
    std::vector<ProcessorConfig> processors;
    size_t max_finished_traces = 1000;
    bool auto_flush_on_shutdown = true;
    
    TracingConfig() = default;
    
    /**
     * Load configuration from JSON
     */
    static TracingConfig from_json(const nlohmann::json& json);
    
    /**
     * Convert configuration to JSON
     */
    nlohmann::json to_json() const;
};

/**
 * Tracing system setup and management
 */
class TracingSetup {
private:
    static std::unique_ptr<TracingProcessor> current_processor_;
    static std::unique_ptr<ProcessorFactory> factory_;
    static TracingConfig config_;
    static bool initialized_;
    static std::mutex setup_mutex_;
    
public:
    /**
     * Initialize the tracing system with configuration
     */
    static void initialize(const TracingConfig& config = TracingConfig{});
    
    /**
     * Initialize from JSON configuration
     */
    static void initialize_from_json(const nlohmann::json& config_json);
    
    /**
     * Initialize from configuration file
     */
    static void initialize_from_file(const std::string& config_file_path);
    
    /**
     * Shutdown the tracing system
     */
    static void shutdown();
    
    /**
     * Check if tracing is initialized
     */
    static bool is_initialized() { return initialized_; }
    
    /**
     * Get the current processor
     */
    static std::shared_ptr<TracingProcessor> get_processor();
    
    /**
     * Set a custom processor
     */
    static void set_processor(std::unique_ptr<TracingProcessor> processor);
    
    /**
     * Set a custom processor factory
     */
    static void set_processor_factory(std::unique_ptr<ProcessorFactory> factory);
    
    /**
     * Get the current configuration
     */
    static const TracingConfig& get_config() { return config_; }
    
    /**
     * Update configuration
     */
    static void update_config(const TracingConfig& config);
    
    /**
     * Enable/disable tracing
     */
    static void set_enabled(bool enabled);
    
    /**
     * Check if tracing is enabled
     */
    static bool is_enabled() { return config_.enabled; }
    
    /**
     * Flush all trace data
     */
    static void flush();
    
    /**
     * Get system status
     */
    static nlohmann::json get_status();
    
    /**
     * Get processor factory
     */
    static ProcessorFactory& get_factory();
};

/**
 * RAII setup guard for automatic initialization and cleanup
 */
class TracingSetupGuard {
private:
    bool should_cleanup_;
    
public:
    explicit TracingSetupGuard(const TracingConfig& config = TracingConfig{})
        : should_cleanup_(true) {
        TracingSetup::initialize(config);
    }
    
    explicit TracingSetupGuard(const nlohmann::json& config_json)
        : should_cleanup_(true) {
        TracingSetup::initialize_from_json(config_json);
    }
    
    ~TracingSetupGuard() {
        if (should_cleanup_) {
            TracingSetup::shutdown();
        }
    }
    
    void disable_cleanup() { should_cleanup_ = false; }
    
    // Non-copyable, movable
    TracingSetupGuard(const TracingSetupGuard&) = delete;
    TracingSetupGuard& operator=(const TracingSetupGuard&) = delete;
    
    TracingSetupGuard(TracingSetupGuard&& other) noexcept
        : should_cleanup_(other.should_cleanup_) {
        other.should_cleanup_ = false;
    }
    
    TracingSetupGuard& operator=(TracingSetupGuard&& other) noexcept {
        if (this != &other) {
            if (should_cleanup_) {
                TracingSetup::shutdown();
            }
            should_cleanup_ = other.should_cleanup_;
            other.should_cleanup_ = false;
        }
        return *this;
    }
};

/**
 * Convenience functions for quick setup
 */
namespace quick_setup {

/**
 * Setup console logging
 */
void console(bool pretty_print = true, bool use_stderr = false);

/**
 * Setup file logging
 */
void file(const std::string& file_path, bool append_mode = true);

/**
 * Setup HTTP export
 */
void http(const std::string& endpoint_url, const std::string& api_key = "");

/**
 * Setup memory storage (for testing)
 */
void memory(size_t max_items = 10000);

/**
 * Setup multiple processors
 */
void multi(const std::vector<ProcessorConfig>& processor_configs);

/**
 * Disable tracing
 */
void disabled();

} // namespace quick_setup

} // namespace tracing
} // namespace openai_agents