#include "_transforms.h"
#include <cctype>
#include <sstream>
#include <iterator>

namespace openai_agents {
namespace util {

std::string transform_string_function_style(const std::string& name) {
    std::string result = name;
    
    // Replace spaces with underscores
    std::replace(result.begin(), result.end(), ' ', '_');
    
    // Replace non-alphanumeric characters with underscores
    std::regex non_alnum_regex("[^a-zA-Z0-9]");
    result = std::regex_replace(result, non_alnum_regex, "_");
    
    // Convert to lowercase
    return to_lowercase(result);
}

std::string transform_string_camel_case(const std::string& name) {
    auto words = extract_words(name);
    if (words.empty()) return "";
    
    std::string result = to_lowercase(words[0]);
    for (size_t i = 1; i < words.size(); ++i) {
        if (!words[i].empty()) {
            result += to_uppercase(words[i].substr(0, 1)) + to_lowercase(words[i].substr(1));
        }
    }
    return result;
}

std::string transform_string_pascal_case(const std::string& name) {
    auto words = extract_words(name);
    std::string result;
    
    for (const auto& word : words) {
        if (!word.empty()) {
            result += to_uppercase(word.substr(0, 1)) + to_lowercase(word.substr(1));
        }
    }
    return result;
}

std::string transform_string_kebab_case(const std::string& name) {
    auto words = extract_words(name);
    std::vector<std::string> lowercase_words;
    
    for (const auto& word : words) {
        lowercase_words.push_back(to_lowercase(word));
    }
    
    return join_strings(lowercase_words, "-");
}

std::string transform_string_constant_case(const std::string& name) {
    auto words = extract_words(name);
    std::vector<std::string> uppercase_words;
    
    for (const auto& word : words) {
        uppercase_words.push_back(to_uppercase(word));
    }
    
    return join_strings(uppercase_words, "_");
}

std::string normalize_whitespace(const std::string& text) {
    std::string result = trim(text);
    
    // Replace multiple whitespace characters with single spaces
    std::regex whitespace_regex("\\s+");
    result = std::regex_replace(result, whitespace_regex, " ");
    
    return result;
}

std::string clean_text(const std::string& text) {
    std::string result = text;
    
    // Normalize line endings to \n
    result = replace_all(result, "\r\n", "\n");
    result = replace_all(result, "\r", "\n");
    
    // Remove trailing whitespace from each line
    std::regex trailing_whitespace("[ \t]+$", std::regex_constants::multiline);
    result = std::regex_replace(result, trailing_whitespace, "");
    
    return normalize_whitespace(result);
}

std::vector<std::string> split_string(const std::string& text, const std::string& delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = text.find(delimiter);
    
    while (end != std::string::npos) {
        result.push_back(text.substr(start, end - start));
        start = end + delimiter.length();
        end = text.find(delimiter, start);
    }
    
    result.push_back(text.substr(start));
    return result;
}

std::vector<std::string> split_string_regex(const std::string& text, const std::string& pattern) {
    std::regex regex_pattern(pattern);
    std::sregex_token_iterator iter(text.begin(), text.end(), regex_pattern, -1);
    std::sregex_token_iterator end;
    
    std::vector<std::string> result;
    for (; iter != end; ++iter) {
        result.push_back(*iter);
    }
    
    return result;
}

std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    
    std::ostringstream oss;
    oss << strings[0];
    
    for (size_t i = 1; i < strings.size(); ++i) {
        oss << delimiter << strings[i];
    }
    
    return oss.str();
}

std::string trim(const std::string& text) {
    return trim_right(trim_left(text));
}

std::string trim_left(const std::string& text) {
    auto start = text.begin();
    while (start != text.end() && std::isspace(*start)) {
        ++start;
    }
    return std::string(start, text.end());
}

std::string trim_right(const std::string& text) {
    auto end = text.end();
    while (end != text.begin() && std::isspace(*(end - 1))) {
        --end;
    }
    return std::string(text.begin(), end);
}

std::string to_lowercase(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string to_uppercase(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.length() >= prefix.length() && 
           text.substr(0, prefix.length()) == prefix;
}

bool ends_with(const std::string& text, const std::string& suffix) {
    return text.length() >= suffix.length() && 
           text.substr(text.length() - suffix.length()) == suffix;
}

std::string replace_all(const std::string& text, const std::string& from, const std::string& to) {
    std::string result = text;
    size_t pos = 0;
    
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    
    return result;
}

std::string replace_regex(const std::string& text, const std::string& pattern, const std::string& replacement) {
    std::regex regex_pattern(pattern);
    return std::regex_replace(text, regex_pattern, replacement);
}

std::string escape_regex(const std::string& text) {
    const std::string special_chars = R"(\.^$|()[]{}*+?\\)";
    std::string result;
    
    for (char c : text) {
        if (special_chars.find(c) != std::string::npos) {
            result += '\\';
        }
        result += c;
    }
    
    return result;
}

std::vector<std::string> extract_words(const std::string& text) {
    std::regex word_regex("[a-zA-Z0-9]+");
    std::sregex_iterator iter(text.begin(), text.end(), word_regex);
    std::sregex_iterator end;
    
    std::vector<std::string> words;
    for (; iter != end; ++iter) {
        words.push_back(iter->str());
    }
    
    return words;
}

std::string slugify(const std::string& text, const std::string& separator) {
    // Convert to lowercase and extract words
    auto words = extract_words(to_lowercase(text));
    
    // Join with separator
    return join_strings(words, separator);
}

std::string truncate(const std::string& text, size_t max_length, const std::string& suffix) {
    if (text.length() <= max_length) {
        return text;
    }
    
    if (max_length <= suffix.length()) {
        return suffix.substr(0, max_length);
    }
    
    return text.substr(0, max_length - suffix.length()) + suffix;
}

std::string sanitize_filename(const std::string& filename) {
    // Characters not allowed in filenames on most systems
    const std::string forbidden_chars = R"(<>:"/\|?*)"";
    std::string result = filename;
    
    // Replace forbidden characters with underscores
    for (char c : forbidden_chars) {
        std::replace(result.begin(), result.end(), c, '_');
    }
    
    // Remove control characters
    result.erase(std::remove_if(result.begin(), result.end(), 
                                [](char c) { return std::iscntrl(c); }), 
                 result.end());
    
    // Trim whitespace and dots (problematic on Windows)
    result = trim(result);
    while (!result.empty() && result.back() == '.') {
        result.pop_back();
    }
    
    // Ensure it's not empty
    if (result.empty()) {
        result = "file";
    }
    
    return result;
}

std::string to_cpp_identifier(const std::string& name) {
    if (name.empty()) {
        return "identifier";
    }
    
    std::string result;
    
    // First character must be letter or underscore
    char first = name[0];
    if (std::isalpha(first) || first == '_') {
        result += first;
    } else {
        result += '_';
    }
    
    // Subsequent characters can be letters, digits, or underscores
    for (size_t i = 1; i < name.length(); ++i) {
        char c = name[i];
        if (std::isalnum(c) || c == '_') {
            result += c;
        } else {
            result += '_';
        }
    }
    
    return result;
}

std::string indent_text(const std::string& text, int levels, const std::string& indent_string) {
    if (levels <= 0) return text;
    
    std::string indent;
    for (int i = 0; i < levels; ++i) {
        indent += indent_string;
    }
    
    auto lines = split_string(text, "\n");
    std::vector<std::string> indented_lines;
    
    for (const auto& line : lines) {
        indented_lines.push_back(indent + line);
    }
    
    return join_strings(indented_lines, "\n");
}

std::string word_wrap(const std::string& text, size_t width) {
    if (width == 0) return text;
    
    auto words = split_string(text, " ");
    std::vector<std::string> lines;
    std::string current_line;
    
    for (const auto& word : words) {
        if (current_line.empty()) {
            current_line = word;
        } else if (current_line.length() + 1 + word.length() <= width) {
            current_line += " " + word;
        } else {
            lines.push_back(current_line);
            current_line = word;
        }
    }
    
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }
    
    return join_strings(lines, "\n");
}

std::string substitute_template(
    const std::string& template_str,
    const std::unordered_map<std::string, std::string>& substitutions
) {
    std::string result = template_str;
    
    // Match ${key} pattern
    std::regex placeholder_regex(R"(\$\{([^}]+)\})");
    std::sregex_iterator iter(template_str.begin(), template_str.end(), placeholder_regex);
    std::sregex_iterator end;
    
    // Process matches from end to beginning to avoid offset issues
    std::vector<std::pair<size_t, size_t>> matches;
    std::vector<std::string> keys;
    
    for (; iter != end; ++iter) {
        matches.push_back({iter->position(), iter->length()});
        keys.push_back(iter->str(1)); // Capture group 1 (the key)
    }
    
    // Process in reverse order
    for (int i = static_cast<int>(matches.size()) - 1; i >= 0; --i) {
        const auto& key = keys[i];
        auto substitution_iter = substitutions.find(key);
        if (substitution_iter != substitutions.end()) {
            result.replace(matches[i].first, matches[i].second, substitution_iter->second);
        }
    }
    
    return result;
}

} // namespace util
} // namespace openai_agents