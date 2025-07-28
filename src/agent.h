#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <any>

namespace openai_agents {

// Forward declarations
class RunContextWrapper;
class Usage;

/**
 * Base class for all agent implementations
 */
class AgentBase {
public:
    virtual ~AgentBase() = default;
    
    /**
     * Run the agent with the given input
     */
    virtual std::any run(const std::any& input) = 0;
    
    /**
     * Get the agent's name
     */
    virtual std::string get_name() const = 0;
};

/**
 * Generic Agent class template
 */
template<typename OutputType>
class Agent : public AgentBase {
public:
    Agent(const std::string& name) : name_(name) {}
    
    std::string get_name() const override { return name_; }
    
    /**
     * Run the agent and return typed output
     */
    virtual OutputType run_typed(const std::any& input) = 0;
    
    std::any run(const std::any& input) override {
        return run_typed(input);
    }

private:
    std::string name_;
};

/**
 * Function signature for tools to final output conversion
 */
struct ToolsToFinalOutputResult {
    bool is_final_output;
    std::any final_output;
};

using ToolsToFinalOutputFunction = std::function<ToolsToFinalOutputResult(
    std::shared_ptr<RunContextWrapper>, 
    const std::vector<std::any>&
)>;

} // namespace openai_agents