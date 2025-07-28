#pragma once

/**
 * Input/output guardrails
 */

#include <string>
#include <memory>
#include <functional>

namespace openai_agents {

// Forward declarations
class RunContextWrapper;

// Guardrail result types
struct InputGuardrailResult {
    bool passed;
    std::string message;
    std::shared_ptr<class InputGuardrail> guardrail;
};

struct OutputGuardrailResult {
    bool passed;
    std::string message;
    std::shared_ptr<class OutputGuardrail> guardrail;
};

// Guardrail function output
struct GuardrailFunctionOutput {
    bool allow;
    std::string reason;
};

// Base guardrail classes
class InputGuardrail {
public:
    virtual ~InputGuardrail() = default;
    virtual InputGuardrailResult check(const std::any& input, std::shared_ptr<RunContextWrapper> context) = 0;
    virtual std::string get_name() const = 0;
};

class OutputGuardrail {
public:
    virtual ~OutputGuardrail() = default;
    virtual OutputGuardrailResult check(const std::any& output, std::shared_ptr<RunContextWrapper> context) = 0;
    virtual std::string get_name() const = 0;
};

// Guardrail factory functions
std::shared_ptr<InputGuardrail> input_guardrail(
    const std::string& name,
    std::function<GuardrailFunctionOutput(const std::any&, std::shared_ptr<RunContextWrapper>)> check_func
);

std::shared_ptr<OutputGuardrail> output_guardrail(
    const std::string& name,
    std::function<GuardrailFunctionOutput(const std::any&, std::shared_ptr<RunContextWrapper>)> check_func
);

} // namespace openai_agents