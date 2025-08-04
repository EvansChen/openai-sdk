#pragma once

/**
 * Pretty Printing Utilities for OpenAI Agents Framework
 * 
 * This module provides utilities for formatting and displaying
 * various framework objects in a human-readable format.
 */

#include "_transforms.h"
#include <string>
#include <sstream>
#include <type_traits>
#include <memory>

namespace openai_agents {

// Forward declarations
struct RunResult;
struct RunResultBase;
struct RunResultStreaming;
struct RunErrorDetails;

namespace util {

/**
 * Indent text by a specified number of levels
 * 
 * @param text The text to indent
 * @param indent_level The number of indentation levels
 * @param indent_string The string to use for each indentation level
 * @return The indented text
 */
std::string indent(const std::string& text, int indent_level, const std::string& indent_string = "  ");

/**
 * Convert final output to a readable string representation
 * 
 * @param result The result object containing the final output
 * @return String representation of the final output
 */
std::string final_output_str(const RunResultBase& result);

/**
 * Pretty print a RunResult object
 * 
 * @param result The RunResult to format
 * @return Human-readable string representation of the result
 * 
 * @example Output format:
 * ```
 * RunResult:
 * - Last agent: Agent(name="my_agent", ...)
 * - Final output (string):
 *   Hello, world!
 * - 5 new item(s)
 * - 2 raw response(s)
 * - 1 input guardrail result(s)
 * - 0 output guardrail result(s)
 * (See `RunResult` for more details)
 * ```
 */
std::string pretty_print_result(const RunResult& result);

/**
 * Pretty print a RunErrorDetails object
 * 
 * @param result The RunErrorDetails to format
 * @return Human-readable string representation of the error details
 * 
 * @example Output format:
 * ```
 * RunErrorDetails:
 * - Last agent: Agent(name="my_agent", ...)
 * - 3 new item(s)
 * - 1 raw response(s)
 * - 1 input guardrail result(s)
 * (See `RunErrorDetails` for more details)
 * ```
 */
std::string pretty_print_run_error_details(const RunErrorDetails& result);

/**
 * Pretty print a RunResultStreaming object
 * 
 * @param result The RunResultStreaming to format
 * @return Human-readable string representation of the streaming result
 * 
 * @example Output format:
 * ```
 * RunResultStreaming:
 * - Current agent: Agent(name="my_agent", ...)
 * - Current turn: 3
 * - Max turns: 10
 * - Is complete: false
 * - Final output (string):
 *   Partial result...
 * - 7 new item(s)
 * - 3 raw response(s)
 * - 1 input guardrail result(s)
 * - 0 output guardrail result(s)
 * (See `RunResultStreaming` for more details)
 * ```
 */
std::string pretty_print_run_result_streaming(const RunResultStreaming& result);

/**
 * Generic pretty printer for objects that support << operator
 * 
 * @tparam T The type of object to print
 * @param obj The object to print
 * @return String representation of the object
 */
template<typename T>
std::string pretty_print(const T& obj) {
    std::ostringstream oss;
    oss << obj;
    return oss.str();
}

/**
 * Pretty print a container (vector, list, etc.)
 * 
 * @tparam Container The container type
 * @param container The container to print
 * @param separator The separator between elements
 * @param prefix The prefix for the output
 * @param suffix The suffix for the output
 * @return String representation of the container
 */
template<typename Container>
std::string pretty_print_container(
    const Container& container,
    const std::string& separator = ", ",
    const std::string& prefix = "[",
    const std::string& suffix = "]"
) {
    std::ostringstream oss;
    oss << prefix;
    
    auto it = container.begin();
    if (it != container.end()) {
        oss << pretty_print(*it);
        ++it;
        
        for (; it != container.end(); ++it) {
            oss << separator << pretty_print(*it);
        }
    }
    
    oss << suffix;
    return oss.str();
}

/**
 * Pretty print a map/dictionary
 * 
 * @tparam Map The map type
 * @param map The map to print
 * @param pair_separator The separator between key and value
 * @param entry_separator The separator between entries
 * @param prefix The prefix for the output
 * @param suffix The suffix for the output
 * @return String representation of the map
 */
template<typename Map>
std::string pretty_print_map(
    const Map& map,
    const std::string& pair_separator = ": ",
    const std::string& entry_separator = ", ",
    const std::string& prefix = "{",
    const std::string& suffix = "}"
) {
    std::ostringstream oss;
    oss << prefix;
    
    auto it = map.begin();
    if (it != map.end()) {
        oss << pretty_print(it->first) << pair_separator << pretty_print(it->second);
        ++it;
        
        for (; it != map.end(); ++it) {
            oss << entry_separator << pretty_print(it->first) 
                << pair_separator << pretty_print(it->second);
        }
    }
    
    oss << suffix;
    return oss.str();
}

/**
 * Format a value with a label for display
 * 
 * @tparam T The type of value
 * @param label The label for the value
 * @param value The value to format
 * @param indent_level The indentation level
 * @return Formatted string with label and value
 */
template<typename T>
std::string format_labeled_value(
    const std::string& label,
    const T& value,
    int indent_level = 0
) {
    std::string result = indent("- " + label + ": ", indent_level);
    result += pretty_print(value);
    return result;
}

/**
 * Format a multiline value with a label
 * 
 * @param label The label for the value
 * @param value The multiline value
 * @param indent_level The indentation level
 * @return Formatted string with label and indented value
 */
std::string format_labeled_multiline_value(
    const std::string& label,
    const std::string& value,
    int indent_level = 0
);

/**
 * Format a size/count with appropriate label
 * 
 * @param count The count value
 * @param singular The singular form of the label
 * @param plural The plural form of the label (optional, defaults to singular + "s")
 * @return Formatted count string
 */
std::string format_count(
    size_t count,
    const std::string& singular,
    const std::string& plural = ""
);

/**
 * Format a type name for display
 * 
 * @tparam T The type
 * @return Human-readable type name
 */
template<typename T>
std::string format_type_name() {
    std::string name = typeid(T).name();
    // Simple cleanup for common types
    if (name.find("std::") == 0) {
        name = name.substr(5); // Remove "std::" prefix
    }
    return name;
}

/**
 * Format a type name for a value
 * 
 * @tparam T The type of the value
 * @param value The value (used for type deduction)
 * @return Human-readable type name
 */
template<typename T>
std::string format_type_name(const T& value) {
    return format_type_name<T>();
}

/**
 * Create a separator line for visual separation
 * 
 * @param length The length of the separator
 * @param character The character to use for the separator
 * @return The separator string
 */
std::string create_separator(size_t length = 50, char character = '-');

/**
 * Format a title with optional separator
 * 
 * @param title The title text
 * @param add_separator Whether to add a separator line below
 * @return Formatted title string
 */
std::string format_title(const std::string& title, bool add_separator = true);

/**
 * Format a summary box with border
 * 
 * @param content The content to put in the box
 * @param title Optional title for the box
 * @param width The width of the box (0 for auto)
 * @return Formatted box string
 */
std::string format_box(
    const std::string& content,
    const std::string& title = "",
    size_t width = 0
);

/**
 * Format an error message with context
 * 
 * @param error_type The type of error
 * @param message The error message
 * @param context Optional context information
 * @return Formatted error string
 */
std::string format_error(
    const std::string& error_type,
    const std::string& message,
    const std::string& context = ""
);

/**
 * Format duration in human-readable format
 * 
 * @param milliseconds Duration in milliseconds
 * @return Human-readable duration string
 */
std::string format_duration(long long milliseconds);

/**
 * Format file size in human-readable format
 * 
 * @param bytes Size in bytes
 * @return Human-readable size string
 */
std::string format_file_size(size_t bytes);

} // namespace util
} // namespace openai_agents