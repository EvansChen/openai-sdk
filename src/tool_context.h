#pragma once

/**
 * Tool context for agent tool execution
 */

#include <string>
#include <map>
#include <any>
#include <memory>
#include <optional>
#include <functional>

namespace openai_agents {

// Forward declarations
class Agent;
class RunContext;
class Logger;
class Tool;

// Tool execution result
struct ToolExecutionResult {
    bool success;
    std::any result;
    std::optional<std::string> error_message;
    std::chrono::milliseconds execution_time;
    std::map<std::string, std::any> metadata;
};

// Tool context for providing runtime information to tools
class ToolContext {
private:
    std::shared_ptr<Agent> agent_;
    std::shared_ptr<RunContext> run_context_;
    std::string tool_call_id_;
    std::string tool_name_;
    std::map<std::string, std::any> tool_arguments_;
    std::map<std::string, std::any> context_data_;
    std::shared_ptr<Logger> logger_;

public:
    ToolContext(std::shared_ptr<Agent> agent,
                std::shared_ptr<RunContext> run_context,
                const std::string& tool_call_id,
                const std::string& tool_name,
                const std::map<std::string, std::any>& tool_arguments);

    // Basic properties
    std::shared_ptr<Agent> get_agent() const { return agent_; }
    std::shared_ptr<RunContext> get_run_context() const { return run_context_; }
    const std::string& get_tool_call_id() const { return tool_call_id_; }
    const std::string& get_tool_name() const { return tool_name_; }
    const std::map<std::string, std::any>& get_tool_arguments() const { return tool_arguments_; }

    // Context data management
    void set_data(const std::string& key, const std::any& value) { context_data_[key] = value; }
    std::any get_data(const std::string& key) const;
    bool has_data(const std::string& key) const { return context_data_.find(key) != context_data_.end(); }
    void remove_data(const std::string& key) { context_data_.erase(key); }

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

    // Tool argument helpers
    template<typename T>
    T get_argument(const std::string& name) const {
        auto it = tool_arguments_.find(name);
        if (it != tool_arguments_.end()) {
            return std::any_cast<T>(it->second);
        }
        throw std::runtime_error("Argument not found or type mismatch: " + name);
    }

    template<typename T>
    T get_argument_or(const std::string& name, const T& default_value) const {
        auto it = tool_arguments_.find(name);
        if (it != tool_arguments_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return default_value;
            }
        }
        return default_value;
    }

    bool has_argument(const std::string& name) const {
        return tool_arguments_.find(name) != tool_arguments_.end();
    }

    // Logging
    std::shared_ptr<Logger> get_logger() const { return logger_; }
    void set_logger(std::shared_ptr<Logger> logger) { logger_ = logger; }

    // Convenience logging methods
    void log_debug(const std::string& message) const;
    void log_info(const std::string& message) const;
    void log_warning(const std::string& message) const;
    void log_error(const std::string& message) const;

    // Tool communication
    void call_other_tool(const std::string& tool_name, 
                        const std::map<std::string, std::any>& arguments,
                        std::function<void(const ToolExecutionResult&)> callback = nullptr);

    // State management
    bool is_cancelled() const;
    void check_cancellation() const; // Throws if cancelled
    
    // Utility
    std::map<std::string, std::any> to_dict() const;
    std::string to_string() const;
};

// Tool context factory
class ToolContextFactory {
public:
    static std::shared_ptr<ToolContext> create(
        std::shared_ptr<Agent> agent,
        std::shared_ptr<RunContext> run_context,
        const std::string& tool_call_id,
        const std::string& tool_name,
        const std::map<std::string, std::any>& tool_arguments
    );

    static std::shared_ptr<ToolContext> create_with_logger(
        std::shared_ptr<Agent> agent,
        std::shared_ptr<RunContext> run_context,
        const std::string& tool_call_id,
        const std::string& tool_name,
        const std::map<std::string, std::any>& tool_arguments,
        std::shared_ptr<Logger> logger
    );
};

// Tool context wrapper for read-only access
class ReadOnlyToolContext {
private:
    std::shared_ptr<ToolContext> context_;

public:
    ReadOnlyToolContext(std::shared_ptr<ToolContext> context) : context_(context) {}

    // Read-only access to context properties
    std::shared_ptr<Agent> get_agent() const { return context_->get_agent(); }
    std::shared_ptr<RunContext> get_run_context() const { return context_->get_run_context(); }
    const std::string& get_tool_call_id() const { return context_->get_tool_call_id(); }
    const std::string& get_tool_name() const { return context_->get_tool_name(); }
    const std::map<std::string, std::any>& get_tool_arguments() const { return context_->get_tool_arguments(); }

    // Read-only data access
    std::any get_data(const std::string& key) const { return context_->get_data(key); }
    bool has_data(const std::string& key) const { return context_->has_data(key); }

    template<typename T>
    T get_data_as(const std::string& key) const { return context_->get_data_as<T>(key); }

    template<typename T>
    std::optional<T> try_get_data_as(const std::string& key) const { return context_->try_get_data_as<T>(key); }

    // Read-only argument access
    template<typename T>
    T get_argument(const std::string& name) const { return context_->get_argument<T>(name); }

    template<typename T>
    T get_argument_or(const std::string& name, const T& default_value) const { 
        return context_->get_argument_or<T>(name, default_value); 
    }

    bool has_argument(const std::string& name) const { return context_->has_argument(name); }

    // State queries
    bool is_cancelled() const { return context_->is_cancelled(); }
    
    // Utility
    std::map<std::string, std::any> to_dict() const { return context_->to_dict(); }
    std::string to_string() const { return context_->to_string(); }
};

} // namespace openai_agents