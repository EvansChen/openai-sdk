#pragma once

/**
 * String and Data Transformation Utilities for OpenAI Agents Framework
 * 
 * This module provides various transformation utilities for converting
 * between different string formats, normalizing names, and processing
 * data for use in the framework.
 */

#include <string>
#include <vector>
#include <algorithm>
#include <regex>
#include <unordered_map>
#include <functional>

namespace openai_agents {
namespace util {

/**
 * Transform a string to function-style naming convention
 * 
 * This function converts a string to a format suitable for function names:
 * - Replaces spaces with underscores
 * - Replaces non-alphanumeric characters with underscores
 * - Converts to lowercase
 * 
 * @param name The string to transform
 * @return The transformed string in function-style format
 * 
 * @example
 * transform_string_function_style("My Function Name") -> "my_function_name"
 * transform_string_function_style("Tool-Name (v1.0)") -> "tool_name_v1_0_"
 */
std::string transform_string_function_style(const std::string& name);

/**
 * Transform a string to camelCase
 * 
 * @param name The string to transform
 * @return The string in camelCase format
 * 
 * @example
 * transform_string_camel_case("my function name") -> "myFunctionName"
 * transform_string_camel_case("tool-name") -> "toolName"
 */
std::string transform_string_camel_case(const std::string& name);

/**
 * Transform a string to PascalCase
 * 
 * @param name The string to transform
 * @return The string in PascalCase format
 * 
 * @example
 * transform_string_pascal_case("my function name") -> "MyFunctionName"
 * transform_string_pascal_case("tool-name") -> "ToolName"
 */
std::string transform_string_pascal_case(const std::string& name);

/**
 * Transform a string to kebab-case
 * 
 * @param name The string to transform
 * @return The string in kebab-case format
 * 
 * @example
 * transform_string_kebab_case("My Function Name") -> "my-function-name"
 * transform_string_kebab_case("toolName") -> "tool-name"
 */
std::string transform_string_kebab_case(const std::string& name);

/**
 * Transform a string to CONSTANT_CASE
 * 
 * @param name The string to transform
 * @return The string in CONSTANT_CASE format
 * 
 * @example
 * transform_string_constant_case("my function name") -> "MY_FUNCTION_NAME"
 * transform_string_constant_case("toolName") -> "TOOL_NAME"
 */
std::string transform_string_constant_case(const std::string& name);

/**
 * Normalize whitespace in a string
 * 
 * @param text The text to normalize
 * @return The text with normalized whitespace (single spaces, trimmed)
 */
std::string normalize_whitespace(const std::string& text);

/**
 * Remove extra whitespace and normalize line endings
 * 
 * @param text The text to clean
 * @return The cleaned text
 */
std::string clean_text(const std::string& text);

/**
 * Split a string by delimiter
 * 
 * @param text The text to split
 * @param delimiter The delimiter to split by
 * @return Vector of split strings
 */
std::vector<std::string> split_string(const std::string& text, const std::string& delimiter);

/**
 * Split a string by regex pattern
 * 
 * @param text The text to split
 * @param pattern The regex pattern to split by
 * @return Vector of split strings
 */
std::vector<std::string> split_string_regex(const std::string& text, const std::string& pattern);

/**
 * Join strings with a delimiter
 * 
 * @param strings The strings to join
 * @param delimiter The delimiter to join with
 * @return The joined string
 */
std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter);

/**
 * Trim whitespace from the beginning and end of a string
 * 
 * @param text The text to trim
 * @return The trimmed text
 */
std::string trim(const std::string& text);

/**
 * Trim whitespace from the beginning of a string
 * 
 * @param text The text to trim
 * @return The trimmed text
 */
std::string trim_left(const std::string& text);

/**
 * Trim whitespace from the end of a string
 * 
 * @param text The text to trim
 * @return The trimmed text
 */
std::string trim_right(const std::string& text);

/**
 * Convert string to lowercase
 * 
 * @param text The text to convert
 * @return The lowercase text
 */
std::string to_lowercase(const std::string& text);

/**
 * Convert string to uppercase
 * 
 * @param text The text to convert
 * @return The uppercase text
 */
std::string to_uppercase(const std::string& text);

/**
 * Check if a string starts with a prefix
 * 
 * @param text The text to check
 * @param prefix The prefix to check for
 * @return True if the text starts with the prefix
 */
bool starts_with(const std::string& text, const std::string& prefix);

/**
 * Check if a string ends with a suffix
 * 
 * @param text The text to check
 * @param suffix The suffix to check for
 * @return True if the text ends with the suffix
 */
bool ends_with(const std::string& text, const std::string& suffix);

/**
 * Replace all occurrences of a substring
 * 
 * @param text The text to modify
 * @param from The substring to replace
 * @param to The replacement string
 * @return The text with all occurrences replaced
 */
std::string replace_all(const std::string& text, const std::string& from, const std::string& to);

/**
 * Replace using regex pattern
 * 
 * @param text The text to modify
 * @param pattern The regex pattern to match
 * @param replacement The replacement string
 * @return The text with matches replaced
 */
std::string replace_regex(const std::string& text, const std::string& pattern, const std::string& replacement);

/**
 * Escape special characters for use in regex
 * 
 * @param text The text to escape
 * @return The escaped text
 */
std::string escape_regex(const std::string& text);

/**
 * Extract words from a string (alphanumeric sequences)
 * 
 * @param text The text to extract words from
 * @return Vector of extracted words
 */
std::vector<std::string> extract_words(const std::string& text);

/**
 * Slugify a string (make it URL-friendly)
 * 
 * @param text The text to slugify
 * @param separator The separator to use (default: "-")
 * @return The slugified text
 */
std::string slugify(const std::string& text, const std::string& separator = "-");

/**
 * Truncate a string to a maximum length
 * 
 * @param text The text to truncate
 * @param max_length The maximum length
 * @param suffix The suffix to add if truncated (default: "...")
 * @return The truncated text
 */
std::string truncate(const std::string& text, size_t max_length, const std::string& suffix = "...");

/**
 * Sanitize a string for use as a filename
 * 
 * @param filename The filename to sanitize
 * @return The sanitized filename
 */
std::string sanitize_filename(const std::string& filename);

/**
 * Convert a string to a valid C++ identifier
 * 
 * @param name The name to convert
 * @return A valid C++ identifier
 */
std::string to_cpp_identifier(const std::string& name);

/**
 * Indent text by a number of levels
 * 
 * @param text The text to indent
 * @param levels The number of indentation levels
 * @param indent_string The string to use for each level (default: "  ")
 * @return The indented text
 */
std::string indent_text(const std::string& text, int levels, const std::string& indent_string = "  ");

/**
 * Word wrap text to a specified width
 * 
 * @param text The text to wrap
 * @param width The maximum line width
 * @return The wrapped text
 */
std::string word_wrap(const std::string& text, size_t width);

/**
 * Template-based string substitution
 * 
 * @param template_str The template string with placeholders like ${key}
 * @param substitutions Map of key-value pairs for substitution
 * @return The string with substitutions applied
 */
std::string substitute_template(
    const std::string& template_str,
    const std::unordered_map<std::string, std::string>& substitutions
);

} // namespace util
} // namespace openai_agents