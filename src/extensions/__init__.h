#pragma once

/**
 * Extensions module for OpenAI Agents SDK
 * 
 * This module provides additional functionality and utilities that extend
 * the core agent framework, including handoff management, visualization,
 * and integration with external model providers.
 */

#include "handoff_prompt.h"
#include "handoff_filters.h"
#include "visualization.h"

namespace openai_agents {
namespace extensions {

// Re-export handoff prompt utilities
using RECOMMENDED_PROMPT_PREFIX = RECOMMENDED_PROMPT_PREFIX;
using prompt_with_handoff_instructions = prompt_with_handoff_instructions;
using has_handoff_instructions = has_handoff_instructions;
using extract_base_prompt = extract_base_prompt;
using create_custom_handoff_instructions = create_custom_handoff_instructions;
using HandoffPromptManager = HandoffPromptManager;

// Re-export handoff filter utilities
using HandoffFilter = HandoffFilter;
using remove_all_tools = remove_all_tools;
using remove_tool_types = remove_tool_types;
using remove_function_calls = remove_function_calls;
using remove_computer_calls = remove_computer_calls;
using remove_search_calls = remove_search_calls;
using keep_only_messages = keep_only_messages;
using keep_recent_items = keep_recent_items;
using remove_agent_items = remove_agent_items;
using chain_filters = chain_filters;
using conditional_filter = conditional_filter;
using HandoffFilterBuilder = HandoffFilterBuilder;

// Re-export visualization utilities
using GraphConfig = GraphConfig;
using get_main_graph = get_main_graph;
using get_all_nodes = get_all_nodes;
using get_all_edges = get_all_edges;
using get_agent_graph = get_agent_graph;
using get_tool_graph = get_tool_graph;
using save_dot_file = save_dot_file;
using draw_graph = draw_graph;
using GraphBuilder = GraphBuilder;

// Convenience functions for common use cases

/**
 * Create a standard handoff-enabled agent prompt
 * 
 * @param base_prompt The base agent instructions
 * @param agent_role Optional role description for customization
 * @return Enhanced prompt with handoff instructions
 */
inline std::string create_handoff_agent_prompt(
    const std::string& base_prompt,
    const std::string& agent_role = ""
) {
    if (agent_role.empty()) {
        return prompt_with_handoff_instructions(base_prompt);
    } else {
        std::string custom_instructions = create_custom_handoff_instructions(agent_role);
        return custom_instructions + "\n\n" + base_prompt;
    }
}

/**
 * Create a conversation filter for clean handoffs
 * 
 * This creates a commonly used filter that removes tool calls and keeps
 * only recent conversational messages for cleaner agent handoffs.
 * 
 * @param max_messages Maximum number of recent messages to keep
 * @return Configured handoff filter
 */
inline HandoffFilter create_clean_handoff_filter(size_t max_messages = 10) {
    return HandoffFilterBuilder()
        .remove_tools()
        .messages_only()
        .keep_recent(max_messages)
        .build();
}

/**
 * Create a visualization of an agent network and save as PNG
 * 
 * @param agent The root agent to visualize
 * @param filename Output filename (without .png extension)
 * @param config Optional graph configuration
 * @return true if successful, false otherwise
 */
inline bool visualize_agent_network(
    const std::shared_ptr<AgentBase>& agent,
    const std::string& filename,
    const GraphConfig& config = GraphConfig{}
) {
    return draw_graph(agent, filename, config);
}

/**
 * Quick analysis of an agent network
 * 
 * @param agent The root agent to analyze
 * @return Network statistics and information
 */
inline analysis::NetworkStats analyze_agent_network(const std::shared_ptr<AgentBase>& agent) {
    return analysis::analyze_network(agent);
}

/**
 * Extension registry for managing custom extensions
 */
class ExtensionRegistry {
public:
    /**
     * Register a custom handoff filter
     */
    static void register_filter(const std::string& name, HandoffFilter filter);

    /**
     * Get a registered filter by name
     */
    static HandoffFilter get_filter(const std::string& name);

    /**
     * List all registered filter names
     */
    static std::vector<std::string> list_filters();

    /**
     * Register a custom prompt template
     */
    static void register_prompt_template(const std::string& name, const std::string& template_text);

    /**
     * Get a registered prompt template
     */
    static std::string get_prompt_template(const std::string& name);

    /**
     * List all registered prompt templates
     */
    static std::vector<std::string> list_prompt_templates();

private:
    static std::unordered_map<std::string, HandoffFilter> filters_;
    static std::unordered_map<std::string, std::string> prompt_templates_;
};

} // namespace extensions
} // namespace openai_agents