#pragma once

#include <string>
#include <exception>
#include <memory>
#include <vector>
#include <variant>

namespace openai_agents {

// Forward declarations
class Agent;
class RunItem;
class ModelResponse;
class RunContextWrapper;
class InputGuardrailResult;
class OutputGuardrailResult;

using TResponseInputItem = std::variant<std::string, std::vector<std::string>>;

/**
 * Data collected from an agent run when an exception occurs.
 */
struct RunErrorDetails {
    std::variant<std::string, std::vector<TResponseInputItem>> input;
    std::vector<std::shared_ptr<RunItem>> new_items;
    std::vector<std::shared_ptr<ModelResponse>> raw_responses;
    std::shared_ptr<Agent> last_agent;
    std::shared_ptr<RunContextWrapper> context_wrapper;
    std::vector<std::shared_ptr<InputGuardrailResult>> input_guardrail_results;
    std::vector<std::shared_ptr<OutputGuardrailResult>> output_guardrail_results;

    std::string to_string() const;
};

/**
 * Base class for all exceptions in the Agents SDK.
 */
class AgentsException : public std::exception {
public:
    AgentsException(const std::string& message);
    virtual ~AgentsException() = default;

    const char* what() const noexcept override;
    const std::string& get_message() const;

    std::shared_ptr<RunErrorDetails> run_data;

protected:
    std::string message_;
};

/**
 * Exception raised when the maximum number of turns is exceeded.
 */
class MaxTurnsExceeded : public AgentsException {
public:
    explicit MaxTurnsExceeded(const std::string& message);
};

/**
 * Exception raised when the model does something unexpected,
 * e.g. calling a tool that doesn't exist, or providing malformed JSON.
 */
class ModelBehaviorError : public AgentsException {
public:
    explicit ModelBehaviorError(const std::string& message);
};

/**
 * Exception raised when the user makes an error using the SDK.
 */
class UserError : public AgentsException {
public:
    explicit UserError(const std::string& message);
};

/**
 * Exception raised when an input guardrail tripwire is triggered.
 */
class InputGuardrailTripwireTriggered : public AgentsException {
public:
    explicit InputGuardrailTripwireTriggered(std::shared_ptr<InputGuardrailResult> guardrail_result);
    
    std::shared_ptr<InputGuardrailResult> guardrail_result;
};

/**
 * Exception raised when an output guardrail tripwire is triggered.
 */
class OutputGuardrailTripwireTriggered : public AgentsException {
public:
    explicit OutputGuardrailTripwireTriggered(std::shared_ptr<OutputGuardrailResult> guardrail_result);
    
    std::shared_ptr<OutputGuardrailResult> guardrail_result;
};

} // namespace openai_agents