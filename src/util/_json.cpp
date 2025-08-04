#include "_json.h"
#include <sstream>

namespace openai_agents {
namespace util {

nlohmann::json try_parse_json(const std::string& json_str, const nlohmann::json& fallback_value) {
    try {
        return nlohmann::json::parse(json_str);
    } catch (const nlohmann::json::parse_error&) {
        return fallback_value;
    }
}

bool is_valid_json(const std::string& json_str) {
    try {
        nlohmann::json::parse(json_str);
        return true;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }
}

nlohmann::json extract_partial_json(const std::string& text) {
    // Try to find complete JSON objects or arrays in the text
    // This is a simplified implementation - a more robust version would
    // handle nested structures better
    
    if (text.empty()) {
        return nlohmann::json();
    }
    
    // Find the first '{' or '[' character
    size_t start_pos = 0;
    while (start_pos < text.length() && text[start_pos] != '{' && text[start_pos] != '[') {
        start_pos++;
    }
    
    if (start_pos >= text.length()) {
        return nlohmann::json();
    }
    
    char start_char = text[start_pos];
    char end_char = (start_char == '{') ? '}' : ']';
    
    // Find the matching closing character
    int depth = 0;
    size_t end_pos = start_pos;
    bool in_string = false;
    bool escaped = false;
    
    for (size_t i = start_pos; i < text.length(); i++) {
        char c = text[i];
        
        if (escaped) {
            escaped = false;
            continue;
        }
        
        if (c == '\\') {
            escaped = true;
            continue;
        }
        
        if (c == '"' && !escaped) {
            in_string = !in_string;
            continue;
        }
        
        if (!in_string) {
            if (c == start_char) {
                depth++;
            } else if (c == end_char) {
                depth--;
                if (depth == 0) {
                    end_pos = i;
                    break;
                }
            }
        }
    }
    
    if (depth != 0) {
        // No complete JSON found, return null
        return nlohmann::json();
    }
    
    // Extract the JSON substring
    std::string json_substr = text.substr(start_pos, end_pos - start_pos + 1);
    
    // Try to parse it
    try {
        return nlohmann::json::parse(json_substr);
    } catch (const nlohmann::json::parse_error&) {
        return nlohmann::json();
    }
}

} // namespace util
} // namespace openai_agents