#include "handoff_filters.h"
#include <algorithm>
#include <memory>

namespace openai_agents {
namespace extensions {

HandoffInputData remove_all_tools(const HandoffInputData& handoff_input_data) {
    static const std::unordered_set<std::string> tool_types = {
        "function_call",
        "function_call_output", 
        "computer_call",
        "computer_call_output",
        "file_search_call",
        "web_search_call"
    };
    
    return remove_tool_types(handoff_input_data, tool_types);
}

HandoffInputData remove_tool_types(
    const HandoffInputData& handoff_input_data,
    const std::unordered_set<std::string>& tool_types
) {
    HandoffInputData result = handoff_input_data;
    
    // Filter input history if it's a vector of response items
    if (std::holds_alternative<std::vector<std::shared_ptr<ResponseInputItem>>>(result.input_history)) {
        auto& history = std::get<std::vector<std::shared_ptr<ResponseInputItem>>>(result.input_history);
        history = detail::remove_tool_types_from_input(history, tool_types);
    }
    
    // Filter pre_handoff_items
    result.pre_handoff_items = detail::remove_tools_from_items(result.pre_handoff_items);
    
    // Filter new_items
    result.new_items = detail::remove_tools_from_items(result.new_items);
    
    return result;
}

HandoffInputData remove_function_calls(const HandoffInputData& handoff_input_data) {
    static const std::unordered_set<std::string> function_types = {
        "function_call",
        "function_call_output"
    };
    
    return remove_tool_types(handoff_input_data, function_types);
}

HandoffInputData remove_computer_calls(const HandoffInputData& handoff_input_data) {
    static const std::unordered_set<std::string> computer_types = {
        "computer_call",
        "computer_call_output"
    };
    
    return remove_tool_types(handoff_input_data, computer_types);
}

HandoffInputData remove_search_calls(const HandoffInputData& handoff_input_data) {
    static const std::unordered_set<std::string> search_types = {
        "file_search_call",
        "web_search_call"
    };
    
    return remove_tool_types(handoff_input_data, search_types);
}

HandoffInputData keep_only_messages(const HandoffInputData& handoff_input_data) {
    HandoffInputData result = handoff_input_data;
    
    // Filter to keep only message items
    auto is_message_item = [](const std::shared_ptr<RunItem>& item) -> bool {
        return std::dynamic_pointer_cast<UserMessage>(item) ||
               std::dynamic_pointer_cast<AssistantMessage>(item) ||
               std::dynamic_pointer_cast<SystemMessage>(item);
    };
    
    // Filter pre_handoff_items
    auto& pre_items = result.pre_handoff_items;
    pre_items.erase(
        std::remove_if(pre_items.begin(), pre_items.end(),
            [&is_message_item](const std::shared_ptr<RunItem>& item) {
                return !is_message_item(item);
            }),
        pre_items.end()
    );
    
    // Filter new_items
    auto& new_items = result.new_items;
    new_items.erase(
        std::remove_if(new_items.begin(), new_items.end(),
            [&is_message_item](const std::shared_ptr<RunItem>& item) {
                return !is_message_item(item);
            }),
        new_items.end()
    );
    
    return result;
}

HandoffInputData keep_recent_items(
    const HandoffInputData& handoff_input_data,
    size_t max_items
) {
    HandoffInputData result = handoff_input_data;
    
    // Limit pre_handoff_items
    if (result.pre_handoff_items.size() > max_items) {
        auto start_it = result.pre_handoff_items.end() - max_items;
        result.pre_handoff_items = std::vector<std::shared_ptr<RunItem>>(
            start_it, result.pre_handoff_items.end()
        );
    }
    
    // Limit new_items
    if (result.new_items.size() > max_items) {
        auto start_it = result.new_items.end() - max_items;
        result.new_items = std::vector<std::shared_ptr<RunItem>>(
            start_it, result.new_items.end()
        );
    }
    
    return result;
}

HandoffInputData remove_agent_items(
    const HandoffInputData& handoff_input_data,
    const std::unordered_set<std::string>& agent_names
) {
    HandoffInputData result = handoff_input_data;
    
    auto should_remove = [&agent_names](const std::shared_ptr<RunItem>& item) -> bool {
        // Check if item has an agent_name property and if it's in the exclusion set
        auto handoff_item = std::dynamic_pointer_cast<HandoffCallItem>(item);
        if (handoff_item && agent_names.count(handoff_item->get_target_agent()) > 0) {
            return true;
        }
        
        auto handoff_output = std::dynamic_pointer_cast<HandoffOutputItem>(item);
        if (handoff_output && agent_names.count(handoff_output->get_source_agent()) > 0) {
            return true;
        }
        
        return false;
    };
    
    // Filter pre_handoff_items
    result.pre_handoff_items.erase(
        std::remove_if(result.pre_handoff_items.begin(), result.pre_handoff_items.end(), should_remove),
        result.pre_handoff_items.end()
    );
    
    // Filter new_items
    result.new_items.erase(
        std::remove_if(result.new_items.begin(), result.new_items.end(), should_remove),
        result.new_items.end()
    );
    
    return result;
}

HandoffFilter chain_filters(const std::vector<HandoffFilter>& filters) {
    return [filters](const HandoffInputData& input) -> HandoffInputData {
        HandoffInputData result = input;
        for (const auto& filter : filters) {
            result = filter(result);
        }
        return result;
    };
}

HandoffFilter conditional_filter(
    std::function<bool(const HandoffInputData&)> condition,
    HandoffFilter filter
) {
    return [condition, filter](const HandoffInputData& input) -> HandoffInputData {
        if (condition(input)) {
            return filter(input);
        }
        return input;
    };
}

// HandoffFilterBuilder implementation
HandoffFilterBuilder& HandoffFilterBuilder::remove_tools() {
    filters_.push_back(remove_all_tools);
    return *this;
}

HandoffFilterBuilder& HandoffFilterBuilder::remove_tool_types(
    const std::unordered_set<std::string>& types
) {
    filters_.push_back([types](const HandoffInputData& input) {
        return extensions::remove_tool_types(input, types);
    });
    return *this;
}

HandoffFilterBuilder& HandoffFilterBuilder::keep_recent(size_t max_items) {
    filters_.push_back([max_items](const HandoffInputData& input) {
        return keep_recent_items(input, max_items);
    });
    return *this;
}

HandoffFilterBuilder& HandoffFilterBuilder::messages_only() {
    filters_.push_back(keep_only_messages);
    return *this;
}

HandoffFilterBuilder& HandoffFilterBuilder::exclude_agents(
    const std::unordered_set<std::string>& agent_names
) {
    filters_.push_back([agent_names](const HandoffInputData& input) {
        return remove_agent_items(input, agent_names);
    });
    return *this;
}

HandoffFilterBuilder& HandoffFilterBuilder::add_custom(HandoffFilter filter) {
    filters_.push_back(filter);
    return *this;
}

HandoffFilter HandoffFilterBuilder::build() const {
    return chain_filters(filters_);
}

// Detail namespace implementations
namespace detail {

std::vector<std::shared_ptr<RunItem>> remove_tools_from_items(
    const std::vector<std::shared_ptr<RunItem>>& items
) {
    std::vector<std::shared_ptr<RunItem>> filtered_items;
    
    for (const auto& item : items) {
        if (!is_tool_item(item)) {
            filtered_items.push_back(item);
        }
    }
    
    return filtered_items;
}

std::vector<std::shared_ptr<ResponseInputItem>> remove_tool_types_from_input(
    const std::vector<std::shared_ptr<ResponseInputItem>>& items,
    const std::unordered_set<std::string>& tool_types
) {
    std::vector<std::shared_ptr<ResponseInputItem>> filtered_items;
    
    for (const auto& item : items) {
        std::string item_type = get_item_type(item);
        if (tool_types.find(item_type) == tool_types.end()) {
            filtered_items.push_back(item);
        }
    }
    
    return filtered_items;
}

bool is_tool_item(const std::shared_ptr<RunItem>& item) {
    return std::dynamic_pointer_cast<HandoffCallItem>(item) ||
           std::dynamic_pointer_cast<HandoffOutputItem>(item) ||
           std::dynamic_pointer_cast<ToolCallItem>(item) ||
           std::dynamic_pointer_cast<ToolCallOutputItem>(item);
}

bool is_tool_type(const std::shared_ptr<ResponseInputItem>& item, const std::string& tool_type) {
    return get_item_type(item) == tool_type;
}

std::string get_item_type(const std::shared_ptr<ResponseInputItem>& item) {
    // This would need to be implemented based on the actual ResponseInputItem interface
    // For now, return a placeholder - in real implementation, this would extract the type field
    if (auto msg = std::dynamic_pointer_cast<UserMessage>(item)) {
        return "user_message";
    } else if (auto msg = std::dynamic_pointer_cast<AssistantMessage>(item)) {
        return "assistant_message";
    } else if (auto msg = std::dynamic_pointer_cast<SystemMessage>(item)) {
        return "system_message";
    }
    // Add other type checks based on actual item types
    return "unknown";
}

} // namespace detail

} // namespace extensions
} // namespace openai_agents