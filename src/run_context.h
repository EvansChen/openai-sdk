#pragma once

/**
 * Context for agent execution
 */

#include <string>
#include <map>
#include <any>
#include <memory>
#include <optional>
#include <chrono>
#include <vector>

namespace openai_agents {

// Forward declarations
class Agent;
class Tool;
class Logger;
class Usage;
class Item;

// Run statistics
struct RunStatistics {
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    size_t total_steps;
    size_t tool_calls_made;
    size_t errors_encountered;
    std::chrono::milliseconds total_duration;
    std::chrono::milliseconds model_time;
    std::chrono::milliseconds tool_time;
};

// Run context for tracking execution state
class RunContext {
private:
    std::string run_id_;
    std::shared_ptr<Agent> agent_;
    std::map<std::string, std::any> context_data_;
    std::vector<std::shared_ptr<Item>> message_history_;
    std::shared_ptr<Usage> usage_;
    std::shared_ptr<Logger> logger_;
    RunStatistics stats_;
    bool cancelled_;
    std::optional<std::string> cancellation_reason_;

public:
    RunContext(const std::string& run_id, std::shared_ptr<Agent> agent);

    // Basic properties
    const std::string& get_run_id() const { return run_id_; }
    std::shared_ptr<Agent> get_agent() const { return agent_; }
    
    // Context data management
    void set_data(const std::string& key, const std::any& value) { context_data_[key] = value; }
    std::any get_data(const std::string& key) const;
    bool has_data(const std::string& key) const { return context_data_.find(key) != context_data_.end(); }
    void remove_data(const std::string& key) { context_data_.erase(key); }
    void clear_data() { context_data_.clear(); }

    // Typed data access
    template<typename T>
    T get_data_as(const std::string& key) const {
        auto it = context_data_.find(key);
        if (it != context_data_.end()) {
            return std::any_cast<T>(it->second);
        }
        throw std::runtime_error("Key not found or type mismatch: " + key);
    }

    template<typename T>
    std::optional<T> try_get_data_as(const std::string& key) const {
        auto it = context_data_.find(key);
        if (it != context_data_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    // Message history
    void add_message(std::shared_ptr<Item> message) { message_history_.push_back(message); }
    void add_messages(const std::vector<std::shared_ptr<Item>>& messages);
    const std::vector<std::shared_ptr<Item>>& get_message_history() const { return message_history_; }
    void clear_message_history() { message_history_.clear(); }

    // Usage tracking
    std::shared_ptr<Usage> get_usage() const { return usage_; }
    void set_usage(std::shared_ptr<Usage> usage) { usage_ = usage; }

    // Logging
    std::shared_ptr<Logger> get_logger() const { return logger_; }
    void set_logger(std::shared_ptr<Logger> logger) { logger_ = logger; }

    // Statistics
    RunStatistics& get_stats() { return stats_; }
    const RunStatistics& get_stats() const { return stats_; }
    void update_stats();

    // Cancellation
    bool is_cancelled() const { return cancelled_; }
    void cancel(const std::string& reason = "");
    const std::optional<std::string>& get_cancellation_reason() const { return cancellation_reason_; }

    // Utility methods
    std::chrono::milliseconds get_elapsed_time() const;
    bool is_running() const;
    void start_run();
    void end_run();

    // Debug/inspection
    std::map<std::string, std::any> to_dict() const;
    std::string to_string() const;
};

// Wrapper for passing context to tools and other components
class RunContextWrapper {
private:
    std::shared_ptr<RunContext> context_;

public:
    RunContextWrapper(std::shared_ptr<RunContext> context) : context_(context) {}

    // Delegate to underlying context
    const std::string& get_run_id() const { return context_->get_run_id(); }
    std::shared_ptr<Agent> get_agent() const { return context_->get_agent(); }
    
    std::any get_data(const std::string& key) const { return context_->get_data(key); }
    bool has_data(const std::string& key) const { return context_->has_data(key); }
    
    template<typename T>
    T get_data_as(const std::string& key) const { return context_->get_data_as<T>(key); }
    
    template<typename T>
    std::optional<T> try_get_data_as(const std::string& key) const { return context_->try_get_data_as<T>(key); }

    const std::vector<std::shared_ptr<Item>>& get_message_history() const { return context_->get_message_history(); }
    std::shared_ptr<Usage> get_usage() const { return context_->get_usage(); }
    std::shared_ptr<Logger> get_logger() const { return context_->get_logger(); }
    
    bool is_cancelled() const { return context_->is_cancelled(); }
    const std::optional<std::string>& get_cancellation_reason() const { return context_->get_cancellation_reason(); }

    // Get raw context for full access (use carefully)
    std::shared_ptr<RunContext> get_raw_context() const { return context_; }
};

// Context factory
class RunContextFactory {
public:
    static std::shared_ptr<RunContext> create(std::shared_ptr<Agent> agent, const std::string& run_id = "");
    static std::shared_ptr<RunContext> create_with_logger(std::shared_ptr<Agent> agent, 
                                                         std::shared_ptr<Logger> logger,
                                                         const std::string& run_id = "");
private:
    static std::string generate_run_id();
};

} // namespace openai_agents