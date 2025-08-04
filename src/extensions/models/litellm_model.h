#pragma once

/**
 * LiteLLM model implementation for accessing multiple LLM providers
 * 
 * This class enables using any model via LiteLLM, which provides access to
 * OpenAI, Anthropic, Gemini, Mistral, and many other models through a
 * unified interface. See supported models at: https://docs.litellm.ai/docs/providers
 */

#include "../../models/interface.h"
#include "../../agent_output.h"
#include "../../handoffs.h"
#include "../../items.h"
#include "../../logger.h"
#include "../../model_settings.h"
#include "../../tool.h"
#include "../../tracing/spans.h"
#include "../../usage.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <future>

namespace openai_agents {
namespace extensions {
namespace models {

/**
 * LiteLLM response structure for non-streaming responses
 */
struct LitellmResponse {
    std::string id;
    std::string object;
    int64_t created;
    std::string model;
    std::vector<Choice> choices;
    std::optional<Usage> usage;
    std::unordered_map<std::string, std::any> system_fingerprint;
};

/**
 * LiteLLM message structure
 */
struct LitellmMessage {
    std::string role;
    std::optional<std::string> content;
    std::vector<ToolCall> tool_calls;
    std::unordered_map<std::string, std::any> provider_specific_fields;
    std::optional<std::any> audio;
    std::vector<Annotation> annotations;
};

/**
 * LiteLLM tool call structure
 */
struct LitellmToolCall {
    std::string id;
    std::string type;
    ToolCallFunction function;
};

/**
 * Model implementation using LiteLLM for multi-provider access
 */
class LitellmModel : public openai_agents::Model {
public:
    /**
     * Constructor for LiteLLM model
     * 
     * @param model Model name/identifier (e.g., "gpt-4", "claude-3", "gemini-pro")
     * @param base_url Optional base URL for the model provider
     * @param api_key Optional API key for authentication
     * 
     * @example
     * ```cpp
     * // OpenAI model
     * auto openai_model = std::make_shared<LitellmModel>("gpt-4");
     * 
     * // Anthropic model
     * auto claude_model = std::make_shared<LitellmModel>("claude-3-sonnet-20240229");
     * 
     * // Custom endpoint
     * auto custom_model = std::make_shared<LitellmModel>(
     *     "custom-model", 
     *     "https://api.custom-provider.com", 
     *     "custom-api-key"
     * );
     * ```
     */
    explicit LitellmModel(
        const std::string& model,
        const std::optional<std::string>& base_url = std::nullopt,
        const std::optional<std::string>& api_key = std::nullopt
    );

    /**
     * Get a response from the model (non-streaming)
     * 
     * @param system_instructions Optional system instructions
     * @param input User input or conversation history
     * @param model_settings Model configuration settings
     * @param tools Available tools for the model
     * @param output_schema Optional output schema for structured responses
     * @param handoffs Available agent handoffs
     * @param tracing Tracing configuration
     * @param previous_response_id Optional ID of previous response
     * @param prompt Optional additional prompt data
     * @return Future containing the model response
     */
    std::future<ModelResponse> get_response(
        const std::optional<std::string>& system_instructions,
        const std::variant<std::string, std::vector<std::shared_ptr<ResponseInputItem>>>& input,
        const ModelSettings& model_settings,
        const std::vector<std::shared_ptr<Tool>>& tools,
        const std::optional<std::shared_ptr<AgentOutputSchemaBase>>& output_schema,
        const std::vector<std::shared_ptr<Handoff>>& handoffs,
        const ModelTracing& tracing,
        const std::optional<std::string>& previous_response_id,
        const std::optional<std::any>& prompt = std::nullopt
    ) override;

    /**
     * Stream a response from the model
     * 
     * @param system_instructions Optional system instructions
     * @param input User input or conversation history
     * @param model_settings Model configuration settings
     * @param tools Available tools for the model
     * @param output_schema Optional output schema for structured responses
     * @param handoffs Available agent handoffs
     * @param tracing Tracing configuration
     * @param previous_response_id Optional ID of previous response
     * @param prompt Optional additional prompt data
     * @return Future containing an async iterator of stream events
     */
    std::future<std::unique_ptr<AsyncIterator<ResponseStreamEvent>>> stream_response(
        const std::optional<std::string>& system_instructions,
        const std::variant<std::string, std::vector<std::shared_ptr<ResponseInputItem>>>& input,
        const ModelSettings& model_settings,
        const std::vector<std::shared_ptr<Tool>>& tools,
        const std::optional<std::shared_ptr<AgentOutputSchemaBase>>& output_schema,
        const std::vector<std::shared_ptr<Handoff>>& handoffs,
        const ModelTracing& tracing,
        const std::optional<std::string>& previous_response_id,
        const std::optional<std::any>& prompt = std::nullopt
    ) override;

    /**
     * Get the model name
     */
    const std::string& get_model_name() const;

    /**
     * Get the base URL
     */
    const std::optional<std::string>& get_base_url() const;

    /**
     * Set API key dynamically
     */
    void set_api_key(const std::string& api_key);

    /**
     * Set base URL dynamically
     */
    void set_base_url(const std::string& base_url);

private:
    std::string model_;
    std::optional<std::string> base_url_;
    std::optional<std::string> api_key_;

    /**
     * Internal method to fetch response (handles both streaming and non-streaming)
     */
    std::future<std::variant<LitellmResponse, std::pair<Response, std::unique_ptr<AsyncStream>>>> 
    fetch_response(
        const std::optional<std::string>& system_instructions,
        const std::variant<std::string, std::vector<std::shared_ptr<ResponseInputItem>>>& input,
        const ModelSettings& model_settings,
        const std::vector<std::shared_ptr<Tool>>& tools,
        const std::optional<std::shared_ptr<AgentOutputSchemaBase>>& output_schema,
        const std::vector<std::shared_ptr<Handoff>>& handoffs,
        const std::shared_ptr<tracing::Span>& span,
        const ModelTracing& tracing,
        bool stream = false,
        const std::optional<std::any>& prompt = std::nullopt
    );

    /**
     * Remove NotGiven values from parameters
     */
    std::any remove_not_given(const std::any& value);

    /**
     * Convert model settings to LiteLLM parameters
     */
    std::unordered_map<std::string, std::any> build_litellm_params(
        const std::vector<std::unordered_map<std::string, std::any>>& messages,
        const std::vector<std::unordered_map<std::string, std::any>>& tools,
        const ModelSettings& model_settings,
        const std::optional<std::any>& tool_choice,
        const std::optional<std::any>& response_format,
        bool stream,
        const std::optional<bool>& parallel_tool_calls
    );
};

/**
 * Converter utilities for LiteLLM format
 */
class LitellmConverter {
public:
    /**
     * Convert LiteLLM message to OpenAI format
     */
    static ChatCompletionMessage convert_message_to_openai(const LitellmMessage& message);

    /**
     * Convert LiteLLM tool call to OpenAI format
     */
    static ChatCompletionMessageToolCall convert_tool_call_to_openai(const LitellmToolCall& tool_call);

    /**
     * Convert LiteLLM annotations to OpenAI format
     */
    static std::optional<std::vector<Annotation>> convert_annotations_to_openai(const LitellmMessage& message);

    /**
     * Convert OpenAI messages to LiteLLM format
     */
    static std::vector<std::unordered_map<std::string, std::any>> convert_messages_to_litellm(
        const std::vector<std::unordered_map<std::string, std::any>>& openai_messages
    );

    /**
     * Convert OpenAI tools to LiteLLM format
     */
    static std::vector<std::unordered_map<std::string, std::any>> convert_tools_to_litellm(
        const std::vector<std::unordered_map<std::string, std::any>>& openai_tools
    );
};

/**
 * LiteLLM client wrapper for making API calls
 */
class LitellmClient {
public:
    /**
     * Initialize LiteLLM client
     */
    static void initialize();

    /**
     * Make completion request to LiteLLM
     */
    static std::future<LitellmResponse> completion(
        const std::unordered_map<std::string, std::any>& params
    );

    /**
     * Make streaming completion request to LiteLLM
     */
    static std::future<std::unique_ptr<AsyncStream>> completion_stream(
        const std::unordered_map<std::string, std::any>& params
    );

    /**
     * Check if LiteLLM is available
     */
    static bool is_available();

    /**
     * Get supported providers
     */
    static std::vector<std::string> get_supported_providers();

    /**
     * Get supported models for a provider
     */
    static std::vector<std::string> get_supported_models(const std::string& provider);

private:
    static bool initialized_;
    static std::unordered_map<std::string, std::vector<std::string>> provider_models_;
};

/**
 * Factory function for creating LiteLLM models
 */
std::shared_ptr<LitellmModel> create_litellm_model(
    const std::string& model,
    const std::optional<std::string>& base_url = std::nullopt,
    const std::optional<std::string>& api_key = std::nullopt
);

/**
 * Utility functions for LiteLLM model management
 */
namespace utils {

/**
 * Parse model string to extract provider and model name
 */
std::pair<std::string, std::string> parse_model_string(const std::string& model);

/**
 * Validate model configuration
 */
bool validate_model_config(const std::string& model, const std::optional<std::string>& base_url);

/**
 * Get default API key for a provider
 */
std::optional<std::string> get_default_api_key(const std::string& provider);

/**
 * Get default base URL for a provider
 */
std::optional<std::string> get_default_base_url(const std::string& provider);

} // namespace utils

} // namespace models
} // namespace extensions
} // namespace openai_agents