#pragma once

/**
 * Common handoff input filters for multi-agent systems
 * 
 * This module provides convenience filters for processing handoff input data,
 * allowing agents to selectively remove or modify conversation history
 * before handing off to other agents.
 */

#include "../handoffs.h"
#include "../items.h"
#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>

namespace openai_agents {
namespace extensions {

/**
 * Type alias for handoff filter functions
 * 
 * A handoff filter takes HandoffInputData and returns modified HandoffInputData
 */
using HandoffFilter = std::function<HandoffInputData(const HandoffInputData&)>;

/**
 * Filters out all tool items: file search, web search and function calls+output
 * 
 * This filter removes all tool-related items from the conversation history,
 * providing a clean conversation flow without technical tool interactions.
 * 
 * @param handoff_input_data The input data to filter
 * @return Filtered input data with all tool items removed
 * 
 * @example
 * ```cpp
 * HandoffInputData filtered = remove_all_tools(original_data);
 * // All ToolCallItem, ToolCallOutputItem, HandoffCallItem, etc. are removed
 * ```
 */
HandoffInputData remove_all_tools(const HandoffInputData& handoff_input_data);

/**
 * Filters out specific types of tool items
 * 
 * @param handoff_input_data The input data to filter
 * @param tool_types Set of tool types to remove (e.g., "function_call", "web_search_call")
 * @return Filtered input data with specified tool types removed
 */
HandoffInputData remove_tool_types(
    const HandoffInputData& handoff_input_data,
    const std::unordered_set<std::string>& tool_types
);

/**
 * Filters out function call related items only
 * 
 * @param handoff_input_data The input data to filter
 * @return Filtered input data with function calls removed
 */
HandoffInputData remove_function_calls(const HandoffInputData& handoff_input_data);

/**
 * Filters out computer tool related items only
 * 
 * @param handoff_input_data The input data to filter
 * @return Filtered input data with computer tool calls removed
 */
HandoffInputData remove_computer_calls(const HandoffInputData& handoff_input_data);

/**
 * Filters out search-related items (file search, web search)
 * 
 * @param handoff_input_data The input data to filter
 * @return Filtered input data with search calls removed
 */
HandoffInputData remove_search_calls(const HandoffInputData& handoff_input_data);

/**
 * Keep only messages (user, assistant, system) and remove all other item types
 * 
 * @param handoff_input_data The input data to filter
 * @return Filtered input data with only message items
 */
HandoffInputData keep_only_messages(const HandoffInputData& handoff_input_data);

/**
 * Remove items older than a specified count
 * 
 * @param handoff_input_data The input data to filter
 * @param max_items Maximum number of recent items to keep
 * @return Filtered input data with only recent items
 */
HandoffInputData keep_recent_items(
    const HandoffInputData& handoff_input_data,
    size_t max_items
);

/**
 * Remove items from specific agents
 * 
 * @param handoff_input_data The input data to filter
 * @param agent_names Set of agent names whose items should be removed
 * @return Filtered input data with specified agents' items removed
 */
HandoffInputData remove_agent_items(
    const HandoffInputData& handoff_input_data,
    const std::unordered_set<std::string>& agent_names
);

/**
 * Chain multiple filters together
 * 
 * @param filters Vector of filter functions to apply in sequence
 * @return A combined filter function
 * 
 * @example
 * ```cpp
 * auto combined_filter = chain_filters({
 *     remove_all_tools,
 *     [](const HandoffInputData& data) { return keep_recent_items(data, 10); }
 * });
 * HandoffInputData result = combined_filter(original_data);
 * ```
 */
HandoffFilter chain_filters(const std::vector<HandoffFilter>& filters);

/**
 * Create a conditional filter that applies a filter only if a condition is met
 * 
 * @param condition Function that determines whether to apply the filter
 * @param filter The filter to apply conditionally
 * @return A conditional filter function
 */
HandoffFilter conditional_filter(
    std::function<bool(const HandoffInputData&)> condition,
    HandoffFilter filter
);

/**
 * Utility class for building complex handoff filters
 */
class HandoffFilterBuilder {
public:
    HandoffFilterBuilder() = default;

    /**
     * Add tool removal filter
     */
    HandoffFilterBuilder& remove_tools();

    /**
     * Add specific tool type removal filter
     */
    HandoffFilterBuilder& remove_tool_types(const std::unordered_set<std::string>& types);

    /**
     * Add recent items filter
     */
    HandoffFilterBuilder& keep_recent(size_t max_items);

    /**
     * Add message-only filter
     */
    HandoffFilterBuilder& messages_only();

    /**
     * Add agent exclusion filter
     */
    HandoffFilterBuilder& exclude_agents(const std::unordered_set<std::string>& agent_names);

    /**
     * Add custom filter
     */
    HandoffFilterBuilder& add_custom(HandoffFilter filter);

    /**
     * Build the final combined filter
     */
    HandoffFilter build() const;

private:
    std::vector<HandoffFilter> filters_;
};

// Helper functions for internal use
namespace detail {

/**
 * Remove tool items from a vector of RunItems
 */
std::vector<std::shared_ptr<RunItem>> remove_tools_from_items(
    const std::vector<std::shared_ptr<RunItem>>& items
);

/**
 * Remove specific tool types from response input items
 */
std::vector<std::shared_ptr<ResponseInputItem>> remove_tool_types_from_input(
    const std::vector<std::shared_ptr<ResponseInputItem>>& items,
    const std::unordered_set<std::string>& tool_types
);

/**
 * Check if an item is a tool-related item
 */
bool is_tool_item(const std::shared_ptr<RunItem>& item);

/**
 * Check if an item is of a specific tool type
 */
bool is_tool_type(const std::shared_ptr<ResponseInputItem>& item, const std::string& tool_type);

/**
 * Get the type string from a response input item
 */
std::string get_item_type(const std::shared_ptr<ResponseInputItem>& item);

} // namespace detail

} // namespace extensions
} // namespace openai_agents