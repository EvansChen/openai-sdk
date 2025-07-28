#include "exceptions.h"
#include <sstream>

namespace openai_agents {

std::string RunErrorDetails::to_string() const {
    // Implementation would include pretty printing logic
    // For now, return a basic string representation
    std::stringstream ss;
    ss << "RunErrorDetails[";
    ss << "items_count=" << new_items.size() << ", ";
    ss << "responses_count=" << raw_responses.size();
    ss << "]";
    return ss.str();
}

AgentsException::AgentsException(const std::string& message) 
    : message_(message), run_data(nullptr) {
}

const char* AgentsException::what() const noexcept {
    return message_.c_str();
}

const std::string& AgentsException::get_message() const {
    return message_;
}

MaxTurnsExceeded::MaxTurnsExceeded(const std::string& message) 
    : AgentsException(message) {
}

ModelBehaviorError::ModelBehaviorError(const std::string& message) 
    : AgentsException(message) {
}

UserError::UserError(const std::string& message) 
    : AgentsException(message) {
}

InputGuardrailTripwireTriggered::InputGuardrailTripwireTriggered(
    std::shared_ptr<InputGuardrailResult> guardrail_result) 
    : AgentsException("Guardrail triggered tripwire"), 
      guardrail_result(guardrail_result) {
}

OutputGuardrailTripwireTriggered::OutputGuardrailTripwireTriggered(
    std::shared_ptr<OutputGuardrailResult> guardrail_result) 
    : AgentsException("Guardrail triggered tripwire"), 
      guardrail_result(guardrail_result) {
}

} // namespace openai_agents