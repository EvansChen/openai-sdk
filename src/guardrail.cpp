#include "guardrail.h"
#include "run_context.h"
#include "exceptions.h"

namespace openai_agents {

// Concrete guardrail implementations
class ConcreteInputGuardrail : public InputGuardrail {
private:
    std::string name_;
    std::function<GuardrailFunctionOutput(const std::any&, std::shared_ptr<RunContextWrapper>)> check_func_;

public:
    ConcreteInputGuardrail(
        const std::string& name,
        std::function<GuardrailFunctionOutput(const std::any&, std::shared_ptr<RunContextWrapper>)> check_func
    ) : name_(name), check_func_(check_func) {}

    InputGuardrailResult check(const std::any& input, std::shared_ptr<RunContextWrapper> context) override {
        try {
            auto result = check_func_(input, context);
            return InputGuardrailResult{
                .passed = result.allow,
                .message = result.reason,
                .guardrail = std::static_pointer_cast<InputGuardrail>(shared_from_this())
            };
        } catch (const std::exception& e) {
            return InputGuardrailResult{
                .passed = false,
                .message = "Guardrail error: " + std::string(e.what()),
                .guardrail = std::static_pointer_cast<InputGuardrail>(shared_from_this())
            };
        }
    }

    std::string get_name() const override {
        return name_;
    }
};

class ConcreteOutputGuardrail : public OutputGuardrail {
private:
    std::string name_;
    std::function<GuardrailFunctionOutput(const std::any&, std::shared_ptr<RunContextWrapper>)> check_func_;

public:
    ConcreteOutputGuardrail(
        const std::string& name,
        std::function<GuardrailFunctionOutput(const std::any&, std::shared_ptr<RunContextWrapper>)> check_func
    ) : name_(name), check_func_(check_func) {}

    OutputGuardrailResult check(const std::any& output, std::shared_ptr<RunContextWrapper> context) override {
        try {
            auto result = check_func_(output, context);
            return OutputGuardrailResult{
                .passed = result.allow,
                .message = result.reason,
                .guardrail = std::static_pointer_cast<OutputGuardrail>(shared_from_this())
            };
        } catch (const std::exception& e) {
            return OutputGuardrailResult{
                .passed = false,
                .message = "Guardrail error: " + std::string(e.what()),
                .guardrail = std::static_pointer_cast<OutputGuardrail>(shared_from_this())
            };
        }
    }

    std::string get_name() const override {
        return name_;
    }
};

// Factory functions
std::shared_ptr<InputGuardrail> input_guardrail(
    const std::string& name,
    std::function<GuardrailFunctionOutput(const std::any&, std::shared_ptr<RunContextWrapper>)> check_func
) {
    return std::make_shared<ConcreteInputGuardrail>(name, check_func);
}

std::shared_ptr<OutputGuardrail> output_guardrail(
    const std::string& name,
    std::function<GuardrailFunctionOutput(const std::any&, std::shared_ptr<RunContextWrapper>)> check_func
) {
    return std::make_shared<ConcreteOutputGuardrail>(name, check_func);
}

} // namespace openai_agents