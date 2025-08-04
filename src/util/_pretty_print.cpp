#include "_pretty_print.h"
#include "../result.h"
#include "../exceptions.h"
#include <iomanip>
#include <algorithm>

namespace openai_agents {
namespace util {

std::string indent(const std::string& text, int indent_level, const std::string& indent_string) {
    return indent_text(text, indent_level, indent_string);
}

std::string final_output_str(const RunResultBase& result) {
    const auto& final_output = result.final_output;
    
    if (!final_output.has_value()) {
        return "None";
    }
    
    // Try to handle different types
    try {
        // Check if it's a string
        if (final_output.type() == typeid(std::string)) {
            return std::any_cast<std::string>(final_output);
        }
        
        // Check if it's a const char*
        if (final_output.type() == typeid(const char*)) {
            return std::string(std::any_cast<const char*>(final_output));
        }
        
        // For other types, convert to string representation
        // This is a simplified approach - in a real implementation,
        // you might want to use a more sophisticated serialization system
        
        // Check for numeric types
        if (final_output.type() == typeid(int)) {
            return std::to_string(std::any_cast<int>(final_output));
        }
        if (final_output.type() == typeid(double)) {
            return std::to_string(std::any_cast<double>(final_output));
        }
        if (final_output.type() == typeid(float)) {
            return std::to_string(std::any_cast<float>(final_output));
        }
        if (final_output.type() == typeid(bool)) {
            return std::any_cast<bool>(final_output) ? "true" : "false";
        }
        
        // For other types, return type information
        return std::string("<") + final_output.type().name() + " object>";
        
    } catch (const std::bad_any_cast&) {
        return std::string("<") + final_output.type().name() + " object>";
    }
}

std::string pretty_print_result(const RunResult& result) {
    std::ostringstream output;
    
    output << "RunResult:\n";
    output << format_labeled_value("Last agent", 
        "Agent(name=\"" + result.last_agent->get_name() + "\", ...)");
    output << "\n";
    
    std::string final_output_type = result.final_output.has_value() ? 
        result.final_output.type().name() : "None";
    output << "- Final output (" << final_output_type << "):\n";
    output << indent(final_output_str(result), 1) << "\n";
    
    output << "- " << format_count(result.new_items.size(), "new item") << "\n";
    output << "- " << format_count(result.raw_responses.size(), "raw response") << "\n";
    output << "- " << format_count(result.input_guardrail_results.size(), "input guardrail result") << "\n";
    output << "- " << format_count(result.output_guardrail_results.size(), "output guardrail result") << "\n";
    output << "(See `RunResult` for more details)";
    
    return output.str();
}

std::string pretty_print_run_error_details(const RunErrorDetails& result) {
    std::ostringstream output;
    
    output << "RunErrorDetails:\n";
    output << format_labeled_value("Last agent", 
        "Agent(name=\"" + result.last_agent->get_name() + "\", ...)");
    output << "\n";
    
    output << "- " << format_count(result.new_items.size(), "new item") << "\n";
    output << "- " << format_count(result.raw_responses.size(), "raw response") << "\n";
    output << "- " << format_count(result.input_guardrail_results.size(), "input guardrail result") << "\n";
    output << "(See `RunErrorDetails` for more details)";
    
    return output.str();
}

std::string pretty_print_run_result_streaming(const RunResultStreaming& result) {
    std::ostringstream output;
    
    output << "RunResultStreaming:\n";
    output << format_labeled_value("Current agent", 
        "Agent(name=\"" + result.current_agent->get_name() + "\", ...)");
    output << "\n";
    output << format_labeled_value("Current turn", result.current_turn) << "\n";
    output << format_labeled_value("Max turns", result.max_turns) << "\n";
    output << format_labeled_value("Is complete", result.is_complete ? "true" : "false") << "\n";
    
    std::string final_output_type = result.final_output.has_value() ? 
        result.final_output.type().name() : "None";
    output << "- Final output (" << final_output_type << "):\n";
    output << indent(final_output_str(result), 1) << "\n";
    
    output << "- " << format_count(result.new_items.size(), "new item") << "\n";
    output << "- " << format_count(result.raw_responses.size(), "raw response") << "\n";
    output << "- " << format_count(result.input_guardrail_results.size(), "input guardrail result") << "\n";
    output << "- " << format_count(result.output_guardrail_results.size(), "output guardrail result") << "\n";
    output << "(See `RunResultStreaming` for more details)";
    
    return output.str();
}

std::string format_labeled_multiline_value(
    const std::string& label,
    const std::string& value,
    int indent_level
) {
    std::string result = indent("- " + label + ":\n", indent_level);
    result += indent(value, indent_level + 1);
    return result;
}

std::string format_count(
    size_t count,
    const std::string& singular,
    const std::string& plural
) {
    std::string label = plural.empty() ? (singular + "s") : plural;
    if (count == 1) {
        label = singular;
    }
    
    return std::to_string(count) + " " + label;
}

std::string create_separator(size_t length, char character) {
    return std::string(length, character);
}

std::string format_title(const std::string& title, bool add_separator) {
    std::string result = title;
    if (add_separator) {
        result += "\n" + create_separator(title.length());
    }
    return result;
}

std::string format_box(
    const std::string& content,
    const std::string& title,
    size_t width
) {
    auto lines = split_string(content, "\n");
    
    // Calculate width if not specified
    if (width == 0) {
        width = title.length();
        for (const auto& line : lines) {
            width = std::max(width, line.length());
        }
        width += 4; // Add padding
    }
    
    std::ostringstream result;
    
    // Top border
    result << "+" << std::string(width - 2, '-') << "+\n";
    
    // Title if provided
    if (!title.empty()) {
        size_t padding = (width - 2 - title.length()) / 2;
        result << "|" << std::string(padding, ' ') << title 
               << std::string(width - 2 - padding - title.length(), ' ') << "|\n";
        result << "+" << std::string(width - 2, '-') << "+\n";
    }
    
    // Content lines
    for (const auto& line : lines) {
        size_t content_width = width - 4; // Account for borders and padding
        if (line.length() <= content_width) {
            result << "| " << line 
                   << std::string(content_width - line.length(), ' ') << " |\n";
        } else {
            // Word wrap long lines
            auto wrapped = word_wrap(line, content_width);
            auto wrapped_lines = split_string(wrapped, "\n");
            for (const auto& wrapped_line : wrapped_lines) {
                result << "| " << wrapped_line 
                       << std::string(content_width - wrapped_line.length(), ' ') << " |\n";
            }
        }
    }
    
    // Bottom border
    result << "+" << std::string(width - 2, '-') << "+";
    
    return result.str();
}

std::string format_error(
    const std::string& error_type,
    const std::string& message,
    const std::string& context
) {
    std::ostringstream result;
    result << "ERROR [" << error_type << "]: " << message;
    
    if (!context.empty()) {
        result << "\nContext: " << context;
    }
    
    return result.str();
}

std::string format_duration(long long milliseconds) {
    if (milliseconds < 1000) {
        return std::to_string(milliseconds) + "ms";
    }
    
    double seconds = milliseconds / 1000.0;
    if (seconds < 60) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << seconds << "s";
        return oss.str();
    }
    
    long long minutes = static_cast<long long>(seconds) / 60;
    seconds = std::fmod(seconds, 60.0);
    
    if (minutes < 60) {
        std::ostringstream oss;
        oss << minutes << "m " << std::fixed << std::setprecision(1) << seconds << "s";
        return oss.str();
    }
    
    long long hours = minutes / 60;
    minutes = minutes % 60;
    
    std::ostringstream oss;
    oss << hours << "h " << minutes << "m " << std::fixed << std::setprecision(0) << seconds << "s";
    return oss.str();
}

std::string format_file_size(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    const size_t num_units = sizeof(units) / sizeof(units[0]);
    
    double size = static_cast<double>(bytes);
    size_t unit_index = 0;
    
    while (size >= 1024.0 && unit_index < num_units - 1) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    if (unit_index == 0) {
        oss << static_cast<size_t>(size) << " " << units[unit_index];
    } else {
        oss << std::fixed << std::setprecision(1) << size << " " << units[unit_index];
    }
    
    return oss.str();
}

} // namespace util
} // namespace openai_agents