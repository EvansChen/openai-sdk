#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <any>
#include <memory>
#include <optional>

namespace openai_agents {

// Forward declarations
class RunContextWrapper;

/**
 * Function parameter information
 */
struct ParameterInfo {
    std::string name;
    std::string type;
    std::optional<std::string> description;
    std::any default_value;
    bool required = true;
    bool is_variadic = false;  // for *args
    bool is_keyword = false;   // for **kwargs
};

/**
 * Function documentation extracted from comments or annotations
 */
struct FuncDocumentation {
    std::string name;
    std::optional<std::string> description;
    std::map<std::string, std::string> param_descriptions;
    
    FuncDocumentation(const std::string& func_name) : name(func_name) {}
};

/**
 * Function signature information
 */
class FunctionSignature {
public:
    FunctionSignature(const std::string& name) : name_(name) {}
    
    void add_parameter(const ParameterInfo& param) {
        parameters_.push_back(param);
    }
    
    const std::vector<ParameterInfo>& get_parameters() const { return parameters_; }
    const std::string& get_name() const { return name_; }
    
private:
    std::string name_;
    std::vector<ParameterInfo> parameters_;
};

/**
 * JSON schema for function parameters
 */
struct JsonSchema {
    std::string type = "object";
    std::map<std::string, std::any> properties;
    std::vector<std::string> required;
    bool strict = true;
    
    std::string to_json_string() const;
};

/**
 * Captures the schema for a C++ function, in preparation for sending it to an LLM as a tool.
 */
class FuncSchema {
public:
    FuncSchema(const std::string& name, 
               const std::optional<std::string>& description = std::nullopt);
    
    // Getters
    const std::string& get_name() const { return name_; }
    const std::optional<std::string>& get_description() const { return description_; }
    const JsonSchema& get_params_json_schema() const { return params_json_schema_; }
    const FunctionSignature& get_signature() const { return signature_; }
    bool takes_context() const { return takes_context_; }
    bool is_strict_json_schema() const { return strict_json_schema_; }
    
    // Setters
    void set_description(const std::string& desc) { description_ = desc; }
    void set_takes_context(bool takes) { takes_context_ = takes; }
    void set_strict_json_schema(bool strict) { strict_json_schema_ = strict; }
    
    // Add parameter to the function schema
    void add_parameter(const ParameterInfo& param);
    
    // Convert validated data into arguments suitable for calling the original function
    std::pair<std::vector<std::any>, std::map<std::string, std::any>> 
    to_call_args(const std::map<std::string, std::any>& data) const;
    
    // Build the JSON schema for the parameters
    void build_json_schema();

private:
    std::string name_;
    std::optional<std::string> description_;
    JsonSchema params_json_schema_;
    FunctionSignature signature_;
    bool takes_context_ = false;
    bool strict_json_schema_ = true;
};

/**
 * Function to create a FuncSchema from a function pointer and metadata
 */
template<typename Func>
std::shared_ptr<FuncSchema> function_schema(
    Func func,
    const std::string& name,
    const std::optional<std::string>& description = std::nullopt,
    bool use_strict_json_schema = true
);

/**
 * Docstring parsing styles (for compatibility with Python version)
 */
enum class DocstringStyle {
    GOOGLE,
    NUMPY,
    SPHINX
};

/**
 * Generate function documentation from various sources
 */
FuncDocumentation generate_func_documentation(
    const std::string& func_name,
    const std::optional<std::string>& docstring = std::nullopt,
    DocstringStyle style = DocstringStyle::GOOGLE
);

} // namespace openai_agents