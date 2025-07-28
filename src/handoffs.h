#pragma once

/**
 * Agent handoffs for delegation between agents
 */

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>

namespace openai_agents {

// Forward declarations
class Agent;
class RunContext;

// Handoff result types
struct HandoffResult {
    std::shared_ptr<Agent> agent;
    std::string context;
    std::map<std::string, std::any> metadata;
};

// Handoff validation
struct HandoffValidation {
    bool is_valid;
    std::string error_message;
};

// Handoff filter function types
using HandoffFilter = std::function<bool(std::shared_ptr<Agent>, const std::string&, std::shared_ptr<RunContext>)>;

// Handoff execution
class Handoff {
private:
    std::shared_ptr<Agent> target_agent_;
    std::optional<std::string> context_;
    std::map<std::string, std::any> metadata_;
    std::vector<HandoffFilter> filters_;

public:
    Handoff(std::shared_ptr<Agent> target_agent, 
            const std::optional<std::string>& context = std::nullopt,
            const std::map<std::string, std::any>& metadata = {});

    // Add filters
    void add_filter(HandoffFilter filter);
    void add_filters(const std::vector<HandoffFilter>& filters);

    // Validation
    HandoffValidation validate(std::shared_ptr<RunContext> context) const;

    // Execution
    HandoffResult execute(std::shared_ptr<RunContext> context) const;

    // Getters
    std::shared_ptr<Agent> get_target_agent() const { return target_agent_; }
    const std::optional<std::string>& get_context() const { return context_; }
    const std::map<std::string, std::any>& get_metadata() const { return metadata_; }
};

// Handoff manager for complex delegation scenarios
class HandoffManager {
private:
    std::map<std::string, std::shared_ptr<Agent>> registered_agents_;
    std::vector<HandoffFilter> global_filters_;

public:
    // Agent registration
    void register_agent(const std::string& name, std::shared_ptr<Agent> agent);
    void unregister_agent(const std::string& name);
    std::shared_ptr<Agent> get_agent(const std::string& name) const;

    // Global filters
    void add_global_filter(HandoffFilter filter);
    void clear_global_filters();

    // Handoff creation
    Handoff create_handoff(const std::string& target_agent_name,
                          const std::optional<std::string>& context = std::nullopt,
                          const std::map<std::string, std::any>& metadata = {}) const;

    // Batch handoffs
    std::vector<HandoffResult> execute_handoffs(
        const std::vector<Handoff>& handoffs,
        std::shared_ptr<RunContext> context
    ) const;

    // Utility
    std::vector<std::string> list_agents() const;
    bool has_agent(const std::string& name) const;
};

// Helper functions for common handoff patterns
Handoff handoff_to(std::shared_ptr<Agent> agent, 
                   const std::string& context = "",
                   const std::map<std::string, std::any>& metadata = {});

Handoff conditional_handoff(std::shared_ptr<Agent> agent,
                           std::function<bool(std::shared_ptr<RunContext>)> condition,
                           const std::string& context = "",
                           const std::map<std::string, std::any>& metadata = {});

} // namespace openai_agents