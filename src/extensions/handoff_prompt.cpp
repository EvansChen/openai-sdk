#include "handoff_prompt.h"
#include <algorithm>
#include <sstream>

namespace openai_agents {
namespace extensions {

std::string extract_base_prompt(const std::string& enhanced_prompt) {
    // Look for the end of the recommended prefix
    std::string prefix_marker = "# System context";
    size_t prefix_start = enhanced_prompt.find(prefix_marker);
    
    if (prefix_start == std::string::npos) {
        // No handoff instructions found, return as-is
        return enhanced_prompt;
    }
    
    // Find the end of the handoff instructions section
    // Look for double newline that typically separates the prefix from the main prompt
    size_t search_start = prefix_start;
    size_t double_newline = enhanced_prompt.find("\n\n", search_start);
    
    if (double_newline == std::string::npos) {
        // No clear separation found, assume entire content is handoff instructions
        return "";
    }
    
    // Extract everything after the double newline
    std::string base_prompt = enhanced_prompt.substr(double_newline + 2);
    
    // Trim leading/trailing whitespace
    size_t start = base_prompt.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = base_prompt.find_last_not_of(" \t\n\r");
    return base_prompt.substr(start, end - start + 1);
}

std::string create_custom_handoff_instructions(
    const std::string& agent_role,
    const std::string& additional_instructions
) {
    std::ostringstream oss;
    
    oss << "# System context\n"
        << "You are " << agent_role << " within a multi-agent system called the Agents SDK, "
        << "designed to make agent coordination and execution easy. Agents uses two primary "
        << "abstraction: **Agents** and **Handoffs**. An agent encompasses instructions and tools "
        << "and can hand off a conversation to another agent when appropriate. "
        << "Handoffs are achieved by calling a handoff function, generally named "
        << "`transfer_to_<agent_name>`. Transfers between agents are handled seamlessly in the background;"
        << " do not mention or draw attention to these transfers in your conversation with the user.";
    
    if (!additional_instructions.empty()) {
        oss << "\n\n" << additional_instructions;
    }
    
    oss << "\n";
    
    return oss.str();
}

// HandoffPromptManager implementation
HandoffPromptManager::HandoffPromptManager(const std::string& custom_prefix)
    : prefix_(custom_prefix) {
}

std::string HandoffPromptManager::enhance_prompt(const std::string& prompt) const {
    return prefix_ + "\n\n" + prompt;
}

void HandoffPromptManager::set_custom_prefix(const std::string& custom_prefix) {
    prefix_ = custom_prefix;
}

const std::string& HandoffPromptManager::get_prefix() const {
    return prefix_;
}

void HandoffPromptManager::reset_to_default() {
    prefix_ = RECOMMENDED_PROMPT_PREFIX;
}

} // namespace extensions
} // namespace openai_agents