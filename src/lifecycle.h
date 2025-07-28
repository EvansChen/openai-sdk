#pragma once

/**
 * Agent lifecycle hooks and events
 */

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <any>
#include <map>

namespace openai_agents {

// Forward declarations
class Agent;
class RunContext;
class Item;

// Lifecycle event types
enum class LifecycleEvent {
    BeforeRun,
    AfterRun,
    BeforeStep,
    AfterStep,
    BeforeToolCall,
    AfterToolCall,
    OnError,
    OnComplete,
    OnCancel
};

// Event data
struct LifecycleEventData {
    LifecycleEvent event_type;
    std::shared_ptr<Agent> agent;
    std::shared_ptr<RunContext> context;
    std::map<std::string, std::any> metadata;
    std::any event_specific_data;
};

// Hook function types
using LifecycleHook = std::function<void(const LifecycleEventData&)>;
using AsyncLifecycleHook = std::function<void(const LifecycleEventData&)>; // In real implementation would be async

// Hook registration
struct HookRegistration {
    std::string id;
    LifecycleEvent event_type;
    LifecycleHook hook;
    int priority; // Higher priority runs first
    bool enabled;
};

// Lifecycle manager
class LifecycleManager {
private:
    std::vector<HookRegistration> hooks_;
    mutable size_t next_id_;

public:
    LifecycleManager() : next_id_(0) {}

    // Hook registration
    std::string register_hook(LifecycleEvent event_type, LifecycleHook hook, int priority = 0);
    bool unregister_hook(const std::string& hook_id);
    void enable_hook(const std::string& hook_id);
    void disable_hook(const std::string& hook_id);

    // Event emission
    void emit_event(const LifecycleEventData& event_data) const;
    void emit_event(LifecycleEvent event_type, std::shared_ptr<Agent> agent, 
                   std::shared_ptr<RunContext> context, 
                   const std::map<std::string, std::any>& metadata = {},
                   const std::any& event_specific_data = {}) const;

    // Hook management
    std::vector<std::string> list_hooks(LifecycleEvent event_type = static_cast<LifecycleEvent>(-1)) const;
    void clear_hooks(LifecycleEvent event_type = static_cast<LifecycleEvent>(-1));
    size_t hook_count(LifecycleEvent event_type = static_cast<LifecycleEvent>(-1)) const;

private:
    std::string generate_hook_id() const;
};

// Predefined lifecycle hooks
class StandardLifecycleHooks {
public:
    // Logging hooks
    static LifecycleHook create_logging_hook(const std::string& log_level = "info");
    static LifecycleHook create_performance_hook();
    static LifecycleHook create_error_logging_hook();

    // Validation hooks
    static LifecycleHook create_input_validation_hook();
    static LifecycleHook create_output_validation_hook();

    // Metrics hooks
    static LifecycleHook create_metrics_hook();
    static LifecycleHook create_usage_tracking_hook();

    // Security hooks
    static LifecycleHook create_auth_hook();
    static LifecycleHook create_rate_limiting_hook();

    // Debugging hooks
    static LifecycleHook create_debug_hook();
    static LifecycleHook create_trace_hook();
};

// Event-specific data structures
struct BeforeRunEventData {
    std::vector<std::shared_ptr<Item>> initial_messages;
    std::map<std::string, std::any> run_options;
};

struct AfterRunEventData {
    std::vector<std::shared_ptr<Item>> final_messages;
    bool success;
    std::optional<std::string> error_message;
    std::chrono::milliseconds duration;
};

struct BeforeStepEventData {
    size_t step_number;
    std::vector<std::shared_ptr<Item>> current_messages;
};

struct AfterStepEventData {
    size_t step_number;
    std::vector<std::shared_ptr<Item>> step_messages;
    bool step_success;
};

struct BeforeToolCallEventData {
    std::string tool_name;
    std::map<std::string, std::any> tool_arguments;
    std::string tool_call_id;
};

struct AfterToolCallEventData {
    std::string tool_name;
    std::map<std::string, std::any> tool_arguments;
    std::string tool_call_id;
    std::any tool_result;
    bool tool_success;
    std::chrono::milliseconds tool_duration;
};

// Lifecycle helpers
class LifecycleHelpers {
public:
    // Create event data with type safety
    static LifecycleEventData create_before_run_event(
        std::shared_ptr<Agent> agent,
        std::shared_ptr<RunContext> context,
        const std::vector<std::shared_ptr<Item>>& initial_messages,
        const std::map<std::string, std::any>& run_options = {}
    );

    static LifecycleEventData create_after_run_event(
        std::shared_ptr<Agent> agent,
        std::shared_ptr<RunContext> context,
        const std::vector<std::shared_ptr<Item>>& final_messages,
        bool success,
        const std::optional<std::string>& error_message = std::nullopt,
        std::chrono::milliseconds duration = std::chrono::milliseconds(0)
    );

    // Helper for hook chaining
    static LifecycleHook chain_hooks(const std::vector<LifecycleHook>& hooks);

    // Conditional hooks
    static LifecycleHook conditional_hook(
        std::function<bool(const LifecycleEventData&)> condition,
        LifecycleHook hook
    );
};

// Global lifecycle manager instance
extern LifecycleManager& get_global_lifecycle_manager();

} // namespace openai_agents