#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <variant>
#include <memory>

namespace openai_agents {

/**
 * MCP Tool Choice configuration
 */
struct MCPToolChoice {
    std::string server_label;
    std::string name;
    
    MCPToolChoice(const std::string& label, const std::string& tool_name)
        : server_label(label), name(tool_name) {}
};

/**
 * Tool choice options
 */
using ToolChoice = std::variant<
    std::string,           // "auto", "required", "none", or specific tool name
    MCPToolChoice,         // MCP tool choice
    std::monostate         // null/none
>;

/**
 * Reasoning configuration for reasoning models
 */
struct Reasoning {
    std::optional<int> max_reasoning_tokens;
    // Add other reasoning-related fields as needed
};

/**
 * Response include options
 */
enum class ResponseIncludable {
    USAGE,
    METADATA,
    // Add other includable options as needed
};

/**
 * Settings to use when calling an LLM.
 * 
 * This class holds optional model configuration parameters (e.g. temperature,
 * top_p, penalties, truncation, etc.).
 * 
 * Not all models/providers support all of these parameters, so please check the API documentation
 * for the specific model and provider you are using.
 */
class ModelSettings {
public:
    ModelSettings() = default;
    
    /**
     * Produce a new ModelSettings by overlaying any non-null values from the
     * override on top of this instance.
     */
    ModelSettings resolve(const std::optional<ModelSettings>& override) const;
    
    /**
     * Convert to JSON dictionary representation
     */
    std::map<std::string, std::variant<std::string, int, double, bool>> to_json_dict() const;
    
    // Getters
    std::optional<double> get_temperature() const { return temperature_; }
    std::optional<double> get_top_p() const { return top_p_; }
    std::optional<double> get_frequency_penalty() const { return frequency_penalty_; }
    std::optional<double> get_presence_penalty() const { return presence_penalty_; }
    std::optional<ToolChoice> get_tool_choice() const { return tool_choice_; }
    std::optional<bool> get_parallel_tool_calls() const { return parallel_tool_calls_; }
    std::optional<std::string> get_truncation() const { return truncation_; }
    std::optional<int> get_max_tokens() const { return max_tokens_; }
    std::optional<Reasoning> get_reasoning() const { return reasoning_; }
    std::optional<std::map<std::string, std::string>> get_metadata() const { return metadata_; }
    std::optional<bool> get_store() const { return store_; }
    std::optional<bool> get_include_usage() const { return include_usage_; }
    std::optional<std::vector<ResponseIncludable>> get_response_include() const { return response_include_; }
    std::optional<std::map<std::string, std::string>> get_extra_query() const { return extra_query_; }
    std::optional<std::map<std::string, std::string>> get_extra_body() const { return extra_body_; }
    std::optional<std::map<std::string, std::string>> get_extra_headers() const { return extra_headers_; }
    std::optional<std::map<std::string, std::string>> get_extra_args() const { return extra_args_; }
    
    // Setters
    void set_temperature(std::optional<double> temp) { temperature_ = temp; }
    void set_top_p(std::optional<double> top_p) { top_p_ = top_p; }
    void set_frequency_penalty(std::optional<double> penalty) { frequency_penalty_ = penalty; }
    void set_presence_penalty(std::optional<double> penalty) { presence_penalty_ = penalty; }
    void set_tool_choice(std::optional<ToolChoice> choice) { tool_choice_ = choice; }
    void set_parallel_tool_calls(std::optional<bool> parallel) { parallel_tool_calls_ = parallel; }
    void set_truncation(std::optional<std::string> truncation) { truncation_ = truncation; }
    void set_max_tokens(std::optional<int> tokens) { max_tokens_ = tokens; }
    void set_reasoning(std::optional<Reasoning> reasoning) { reasoning_ = reasoning; }
    void set_metadata(std::optional<std::map<std::string, std::string>> meta) { metadata_ = meta; }
    void set_store(std::optional<bool> store) { store_ = store; }
    void set_include_usage(std::optional<bool> include) { include_usage_ = include; }
    void set_response_include(std::optional<std::vector<ResponseIncludable>> include) { response_include_ = include; }
    void set_extra_query(std::optional<std::map<std::string, std::string>> query) { extra_query_ = query; }
    void set_extra_body(std::optional<std::map<std::string, std::string>> body) { extra_body_ = body; }
    void set_extra_headers(std::optional<std::map<std::string, std::string>> headers) { extra_headers_ = headers; }
    void set_extra_args(std::optional<std::map<std::string, std::string>> args) { extra_args_ = args; }

private:
    std::optional<double> temperature_;                                         ///< The temperature to use when calling the model
    std::optional<double> top_p_;                                              ///< The top_p to use when calling the model
    std::optional<double> frequency_penalty_;                                  ///< The frequency penalty to use when calling the model
    std::optional<double> presence_penalty_;                                   ///< The presence penalty to use when calling the model
    std::optional<ToolChoice> tool_choice_;                                    ///< The tool choice to use when calling the model
    std::optional<bool> parallel_tool_calls_;                                  ///< Controls whether the model can make multiple parallel tool calls
    std::optional<std::string> truncation_;                                    ///< The truncation strategy to use ("auto" or "disabled")
    std::optional<int> max_tokens_;                                            ///< The maximum number of output tokens to generate
    std::optional<Reasoning> reasoning_;                                        ///< Configuration options for reasoning models
    std::optional<std::map<std::string, std::string>> metadata_;              ///< Metadata to include with the model response call
    std::optional<bool> store_;                                                ///< Whether to store the generated model response
    std::optional<bool> include_usage_;                                        ///< Whether to include usage chunk
    std::optional<std::vector<ResponseIncludable>> response_include_;          ///< Additional output data to include
    std::optional<std::map<std::string, std::string>> extra_query_;           ///< Additional query fields
    std::optional<std::map<std::string, std::string>> extra_body_;            ///< Additional body fields
    std::optional<std::map<std::string, std::string>> extra_headers_;         ///< Additional headers
    std::optional<std::map<std::string, std::string>> extra_args_;            ///< Arbitrary keyword arguments
};

} // namespace openai_agents