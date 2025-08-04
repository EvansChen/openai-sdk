#pragma once

/**
 * Handoff prompt utilities for multi-agent systems
 * 
 * This module provides recommended prompt templates and utilities for agents
 * that use handoffs in multi-agent coordination scenarios.
 */

#include <string>

namespace openai_agents {
namespace extensions {

/**
 * A recommended prompt prefix for agents that use handoffs.
 * 
 * We recommend including this or similar instructions in any agents
 * that use handoffs to ensure proper coordination and seamless transfers.
 */
constexpr const char* RECOMMENDED_PROMPT_PREFIX = 
    "# System context\n"
    "You are part of a multi-agent system called the Agents SDK, designed to make agent "
    "coordination and execution easy. Agents uses two primary abstraction: **Agents** and "
    "**Handoffs**. An agent encompasses instructions and tools and can hand off a "
    "conversation to another agent when appropriate. "
    "Handoffs are achieved by calling a handoff function, generally named "
    "`transfer_to_<agent_name>`. Transfers between agents are handled seamlessly in the background;"
    " do not mention or draw attention to these transfers in your conversation with the user.\n";

/**
 * Add recommended instructions to the prompt for agents that use handoffs
 * 
 * @param prompt The base prompt to enhance with handoff instructions
 * @return The enhanced prompt with handoff instructions prepended
 * 
 * @example
 * ```cpp
 * std::string base_prompt = "You are a helpful assistant.";
 * std::string enhanced = prompt_with_handoff_instructions(base_prompt);
 * // Result: RECOMMENDED_PROMPT_PREFIX + "\n\n" + base_prompt
 * ```
 */
inline std::string prompt_with_handoff_instructions(const std::string& prompt) {
    return std::string(RECOMMENDED_PROMPT_PREFIX) + "\n\n" + prompt;
}

/**
 * Check if a prompt already contains handoff instructions
 * 
 * @param prompt The prompt to check
 * @return true if the prompt appears to contain handoff instructions
 */
inline bool has_handoff_instructions(const std::string& prompt) {
    return prompt.find("multi-agent system") != std::string::npos ||
           prompt.find("transfer_to_") != std::string::npos ||
           prompt.find("Handoffs") != std::string::npos;
}

/**
 * Extract the base prompt from a prompt that includes handoff instructions
 * 
 * @param enhanced_prompt The prompt that may include handoff instructions
 * @return The base prompt with handoff instructions removed
 */
std::string extract_base_prompt(const std::string& enhanced_prompt);

/**
 * Customize handoff instructions for specific agent roles
 * 
 * @param agent_role The role or name of the agent
 * @param additional_instructions Additional context-specific instructions
 * @return Customized handoff instructions
 */
std::string create_custom_handoff_instructions(
    const std::string& agent_role,
    const std::string& additional_instructions = ""
);

/**
 * Utility class for managing handoff prompt templates
 */
class HandoffPromptManager {
public:
    /**
     * Constructor with custom template
     * 
     * @param custom_prefix Custom handoff instruction template
     */
    explicit HandoffPromptManager(const std::string& custom_prefix = RECOMMENDED_PROMPT_PREFIX);

    /**
     * Enhance a prompt with handoff instructions
     * 
     * @param prompt The base prompt
     * @return Enhanced prompt with handoff instructions
     */
    std::string enhance_prompt(const std::string& prompt) const;

    /**
     * Set a custom handoff instruction template
     * 
     * @param custom_prefix The new template to use
     */
    void set_custom_prefix(const std::string& custom_prefix);

    /**
     * Get the current handoff instruction template
     * 
     * @return The current template
     */
    const std::string& get_prefix() const;

    /**
     * Reset to the default recommended prefix
     */
    void reset_to_default();

private:
    std::string prefix_;
};

} // namespace extensions
} // namespace openai_agents