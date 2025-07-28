#include "function_schema.h"
#include <sstream>
#include <algorithm>

namespace openai_agents {

std::string JsonSchema::to_json_string() const {
    std::stringstream ss;
    ss << "{";
    ss << "\"type\":\"" << type << "\"";
    
    if (!properties.empty()) {
        ss << ",\"properties\":{";
        bool first = true;
        for (const auto& [key, value] : properties) {
            if (!first) ss << ",";
            ss << "\"" << key << "\":{}"; // Simplified - would need proper JSON serialization
            first = false;
        }
        ss << "}";
    }
    
    if (!required.empty()) {
        ss << ",\"required\":[";
        bool first = true;
        for (const auto& req : required) {
            if (!first) ss << ",";
            ss << "\"" << req << "\"";
            first = false;
        }
        ss << "]";
    }
    
    if (strict) {
        ss << ",\"additionalProperties\":false";
    }
    
    ss << "}";
    return ss.str();
}

FuncSchema::FuncSchema(const std::string& name, 
                       const std::optional<std::string>& description)
    : name_(name), description_(description), signature_(name) {
}

void FuncSchema::add_parameter(const ParameterInfo& param) {
    signature_.add_parameter(param);
    
    // Add to JSON schema properties
    params_json_schema_.properties[param.name] = std::string(param.type);
    
    if (param.required) {
        params_json_schema_.required.push_back(param.name);
    }
}

std::pair<std::vector<std::any>, std::map<std::string, std::any>> 
FuncSchema::to_call_args(const std::map<std::string, std::any>& data) const {
    std::vector<std::any> positional_args;
    std::map<std::string, std::any> keyword_args;
    
    bool seen_var_positional = false;
    
    for (size_t idx = 0; idx < signature_.get_parameters().size(); ++idx) {
        const auto& param = signature_.get_parameters()[idx];
        
        // If the function takes a RunContextWrapper and this is the first parameter, skip it
        if (takes_context_ && idx == 0) {
            continue;
        }
        
        auto it = data.find(param.name);
        std::any value = (it != data.end()) ? it->second : param.default_value;
        
        if (param.is_variadic) {
            // Handle *args
            // For simplicity, assume value is a vector
            seen_var_positional = true;
            // Would need proper handling here based on the actual value type
        } else if (param.is_keyword) {
            // Handle **kwargs
            // Would need proper handling here
        } else {
            // Regular parameter
            if (!seen_var_positional) {
                positional_args.push_back(value);
            } else {
                keyword_args[param.name] = value;
            }
        }
    }
    
    return {positional_args, keyword_args};
}

void FuncSchema::build_json_schema() {
    params_json_schema_.type = "object";
    params_json_schema_.strict = strict_json_schema_;
    // Properties and required fields are already populated in add_parameter
}

FuncDocumentation generate_func_documentation(
    const std::string& func_name,
    const std::optional<std::string>& docstring,
    DocstringStyle style) {
    
    FuncDocumentation doc(func_name);
    
    if (docstring.has_value()) {
        // Simple parsing - in a real implementation, this would be more sophisticated
        const std::string& doc_str = docstring.value();
        
        // Extract first line as description
        size_t first_newline = doc_str.find('\n');
        if (first_newline != std::string::npos) {
            doc.description = doc_str.substr(0, first_newline);
        } else {
            doc.description = doc_str;
        }
        
        // TODO: Parse parameter descriptions based on style
        // This would involve pattern matching for different docstring formats
    }
    
    return doc;
}

} // namespace openai_agents