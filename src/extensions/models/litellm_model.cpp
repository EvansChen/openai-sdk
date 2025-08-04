#include "litellm_model.h"
#include "../../models/chatcmpl_converter.h"
#include "../../models/chatcmpl_stream_handler.h"
#include "../../exceptions.h"
#include "../../util/_json.h"
#include <sstream>
#include <regex>

namespace openai_agents {
namespace extensions {
namespace models {

// Static member initialization
bool LitellmClient::initialized_ = false;
std::unordered_map<std::string, std::vector<std::string>> LitellmClient::provider_models_;

// LitellmModel implementation
LitellmModel::LitellmModel(
    const std::string& model,
    const std::optional<std::string>& base_url,
    const std::optional<std::string>& api_key
) : model_(model), base_url_(base_url), api_key_(api_key) {
    
    if (!LitellmClient::is_available()) {
        throw std::runtime_error(
            "LiteLLM is required to use the LitellmModel. "
            "Please install LiteLLM library and ensure it's available."
        );
    }
    
    LitellmClient::initialize();
}

std::future<ModelResponse> LitellmModel::get_response(
    const std::optional<std::string>& system_instructions,
    const std::variant<std::string, std::vector<std::shared_ptr<ResponseInputItem>>>& input,
    const ModelSettings& model_settings,
    const std::vector<std::shared_ptr<Tool>>& tools,
    const std::optional<std::shared_ptr<AgentOutputSchemaBase>>& output_schema,
    const std::vector<std::shared_ptr<Handoff>>& handoffs,
    const ModelTracing& tracing,
    const std::optional<std::string>& previous_response_id,
    const std::optional<std::any>& prompt
) {
    return std::async(std::launch::async, [=]() -> ModelResponse {
        auto span = tracing::create_generation_span(
            model_, 
            model_settings.to_json_dict(),
            !tracing.is_disabled()
        );
        
        auto response_variant = fetch_response(
            system_instructions, input, model_settings, tools, 
            output_schema, handoffs, span, tracing, false, prompt
        ).get();
        
        if (!std::holds_alternative<LitellmResponse>(response_variant)) {
            throw std::runtime_error("Expected non-streaming response");
        }
        
        auto response = std::get<LitellmResponse>(response_variant);
        
        logger::debug("Received model response from LiteLLM");
        
        // Extract usage information
        Usage usage;
        if (response.usage) {
            usage = *response.usage;
        } else {
            logger::warning("No usage information returned from LiteLLM");
        }
        
        // Update tracing
        if (tracing.include_data() && !response.choices.empty()) {
            span->set_output({response.choices[0].message.to_json()});
        }
        span->set_usage({
            {"input_tokens", usage.input_tokens},
            {"output_tokens", usage.output_tokens}
        });
        
        // Convert response to output items
        auto openai_message = LitellmConverter::convert_message_to_openai(
            response.choices[0].message
        );
        auto items = chatcmpl::Converter::message_to_output_items(openai_message);
        
        return ModelResponse{
            .output = items,
            .usage = usage,
            .response_id = std::nullopt
        };
    });
}

std::future<std::unique_ptr<AsyncIterator<ResponseStreamEvent>>> LitellmModel::stream_response(
    const std::optional<std::string>& system_instructions,
    const std::variant<std::string, std::vector<std::shared_ptr<ResponseInputItem>>>& input,
    const ModelSettings& model_settings,
    const std::vector<std::shared_ptr<Tool>>& tools,
    const std::optional<std::shared_ptr<AgentOutputSchemaBase>>& output_schema,
    const std::vector<std::shared_ptr<Handoff>>& handoffs,
    const ModelTracing& tracing,
    const std::optional<std::string>& previous_response_id,
    const std::optional<std::any>& prompt
) {
    return std::async(std::launch::async, [=]() -> std::unique_ptr<AsyncIterator<ResponseStreamEvent>> {
        auto span = tracing::create_generation_span(
            model_, 
            model_settings.to_json_dict(),
            !tracing.is_disabled()
        );
        
        auto response_variant = fetch_response(
            system_instructions, input, model_settings, tools, 
            output_schema, handoffs, span, tracing, true, prompt
        ).get();
        
        if (!std::holds_alternative<std::pair<Response, std::unique_ptr<AsyncStream>>>(response_variant)) {
            throw std::runtime_error("Expected streaming response");
        }
        
        auto [response, stream] = std::get<std::pair<Response, std::unique_ptr<AsyncStream>>>(std::move(response_variant));
        
        return chatcmpl::ChatCmplStreamHandler::handle_stream(response, std::move(stream));
    });
}

const std::string& LitellmModel::get_model_name() const {
    return model_;
}

const std::optional<std::string>& LitellmModel::get_base_url() const {
    return base_url_;
}

void LitellmModel::set_api_key(const std::string& api_key) {
    api_key_ = api_key;
}

void LitellmModel::set_base_url(const std::string& base_url) {
    base_url_ = base_url;
}

std::future<std::variant<LitellmResponse, std::pair<Response, std::unique_ptr<AsyncStream>>>>
LitellmModel::fetch_response(
    const std::optional<std::string>& system_instructions,
    const std::variant<std::string, std::vector<std::shared_ptr<ResponseInputItem>>>& input,
    const ModelSettings& model_settings,
    const std::vector<std::shared_ptr<Tool>>& tools,
    const std::optional<std::shared_ptr<AgentOutputSchemaBase>>& output_schema,
    const std::vector<std::shared_ptr<Handoff>>& handoffs,
    const std::shared_ptr<tracing::Span>& span,
    const ModelTracing& tracing,
    bool stream,
    const std::optional<std::any>& prompt
) {
    return std::async(std::launch::async, [=]() -> std::variant<LitellmResponse, std::pair<Response, std::unique_ptr<AsyncStream>>> {
        
        // Convert input to messages
        auto converted_messages = chatcmpl::Converter::items_to_messages(input);
        
        // Add system instructions
        if (system_instructions) {
            converted_messages.insert(converted_messages.begin(), {
                {"content", *system_instructions},
                {"role", "system"}
            });
        }
        
        // Update tracing
        if (tracing.include_data()) {
            span->set_input(converted_messages);
        }
        
        // Convert tools and handoffs
        std::vector<std::unordered_map<std::string, std::any>> converted_tools;
        for (const auto& tool : tools) {
            converted_tools.push_back(chatcmpl::Converter::tool_to_openai(*tool));
        }
        for (const auto& handoff : handoffs) {
            converted_tools.push_back(chatcmpl::Converter::convert_handoff_tool(*handoff));
        }
        
        // Prepare parameters
        auto parallel_tool_calls = model_settings.parallel_tool_calls.has_value() && 
                                  !tools.empty() ? model_settings.parallel_tool_calls : std::nullopt;
        
        auto tool_choice = chatcmpl::Converter::convert_tool_choice(model_settings.tool_choice);
        auto response_format = chatcmpl::Converter::convert_response_format(output_schema);
        
        logger::debug("Calling LiteLLM model: " + model_);
        
        // Build LiteLLM parameters
        auto params = build_litellm_params(
            converted_messages, converted_tools, model_settings,
            tool_choice, response_format, stream, parallel_tool_calls
        );
        
        if (stream) {
            // Create response object for streaming
            auto response = Response{
                .id = "fake_response_id",
                .created_at = std::chrono::system_clock::now(),
                .model = model_,
                .object = "response",
                .output = {},
                .tool_choice = tool_choice.has_value() ? 
                    std::any_cast<std::string>(*tool_choice) : "auto",
                .top_p = model_settings.top_p,
                .temperature = model_settings.temperature,
                .tools = {},
                .parallel_tool_calls = parallel_tool_calls.value_or(false)
            };
            
            auto stream = LitellmClient::completion_stream(params).get();
            return std::make_pair(response, std::move(stream));
        } else {
            auto response = LitellmClient::completion(params).get();
            return response;
        }
    });
}

std::any LitellmModel::remove_not_given(const std::any& value) {
    // Implementation would check for NotGiven type and return nullptr
    // For now, return as-is
    return value;
}

std::unordered_map<std::string, std::any> LitellmModel::build_litellm_params(
    const std::vector<std::unordered_map<std::string, std::any>>& messages,
    const std::vector<std::unordered_map<std::string, std::any>>& tools,
    const ModelSettings& model_settings,
    const std::optional<std::any>& tool_choice,
    const std::optional<std::any>& response_format,
    bool stream,
    const std::optional<bool>& parallel_tool_calls
) {
    std::unordered_map<std::string, std::any> params;
    
    params["model"] = model_;
    params["messages"] = messages;
    
    if (!tools.empty()) {
        params["tools"] = tools;
    }
    
    if (model_settings.temperature.has_value()) {
        params["temperature"] = *model_settings.temperature;
    }
    
    if (model_settings.top_p.has_value()) {
        params["top_p"] = *model_settings.top_p;
    }
    
    if (model_settings.frequency_penalty.has_value()) {
        params["frequency_penalty"] = *model_settings.frequency_penalty;
    }
    
    if (model_settings.presence_penalty.has_value()) {
        params["presence_penalty"] = *model_settings.presence_penalty;
    }
    
    if (model_settings.max_tokens.has_value()) {
        params["max_tokens"] = *model_settings.max_tokens;
    }
    
    if (tool_choice.has_value()) {
        params["tool_choice"] = remove_not_given(*tool_choice);
    }
    
    if (response_format.has_value()) {
        params["response_format"] = remove_not_given(*response_format);
    }
    
    if (parallel_tool_calls.has_value()) {
        params["parallel_tool_calls"] = *parallel_tool_calls;
    }
    
    params["stream"] = stream;
    
    if (api_key_.has_value()) {
        params["api_key"] = *api_key_;
    }
    
    if (base_url_.has_value()) {
        params["base_url"] = *base_url_;
    }
    
    // Add extra parameters from model settings
    if (model_settings.extra_args.has_value()) {
        for (const auto& [key, value] : *model_settings.extra_args) {
            params[key] = value;
        }
    }
    
    return params;
}

// LitellmConverter implementation
ChatCompletionMessage LitellmConverter::convert_message_to_openai(const LitellmMessage& message) {
    if (message.role != "assistant") {
        throw ModelBehaviorError("Unsupported role: " + message.role);
    }
    
    std::vector<ChatCompletionMessageToolCall> tool_calls;
    for (const auto& tool_call : message.tool_calls) {
        tool_calls.push_back(convert_tool_call_to_openai(tool_call));
    }
    
    std::optional<std::string> refusal;
    if (message.provider_specific_fields.find("refusal") != message.provider_specific_fields.end()) {
        try {
            refusal = std::any_cast<std::string>(message.provider_specific_fields.at("refusal"));
        } catch (const std::bad_any_cast&) {
            // Ignore if cast fails
        }
    }
    
    return ChatCompletionMessage{
        .content = message.content,
        .refusal = refusal,
        .role = "assistant",
        .annotations = convert_annotations_to_openai(message),
        .audio = message.audio,
        .tool_calls = tool_calls.empty() ? std::nullopt : std::make_optional(tool_calls)
    };
}

ChatCompletionMessageToolCall LitellmConverter::convert_tool_call_to_openai(const LitellmToolCall& tool_call) {
    return ChatCompletionMessageToolCall{
        .id = tool_call.id,
        .type = "function",
        .function = tool_call.function
    };
}

std::optional<std::vector<Annotation>> LitellmConverter::convert_annotations_to_openai(const LitellmMessage& message) {
    if (message.annotations.empty()) {
        return std::nullopt;
    }
    
    return message.annotations;
}

// LitellmClient implementation
void LitellmClient::initialize() {
    if (initialized_) {
        return;
    }
    
    // Initialize provider models mapping
    provider_models_["openai"] = {"gpt-4", "gpt-3.5-turbo", "gpt-4-turbo"};
    provider_models_["anthropic"] = {"claude-3-sonnet-20240229", "claude-3-opus-20240229"};
    provider_models_["google"] = {"gemini-pro", "gemini-pro-vision"};
    provider_models_["mistral"] = {"mistral-large", "mistral-medium"};
    
    initialized_ = true;
}

std::future<LitellmResponse> LitellmClient::completion(
    const std::unordered_map<std::string, std::any>& params
) {
    return std::async(std::launch::async, [params]() -> LitellmResponse {
        // This would interface with actual LiteLLM library
        // For now, return a mock response
        LitellmResponse response;
        response.id = "litellm_response_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        response.object = "chat.completion";
        response.created = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        response.model = std::any_cast<std::string>(params.at("model"));
        
        // Mock choice and usage
        Choice choice;
        choice.index = 0;
        choice.finish_reason = "stop";
        
        LitellmMessage message;
        message.role = "assistant";
        message.content = "This is a mock response from LiteLLM";
        choice.message = message;
        
        response.choices = {choice};
        response.usage = Usage{
            .requests = 1,
            .input_tokens = 10,
            .output_tokens = 15,
            .total_tokens = 25
        };
        
        return response;
    });
}

std::future<std::unique_ptr<AsyncStream>> LitellmClient::completion_stream(
    const std::unordered_map<std::string, std::any>& params
) {
    return std::async(std::launch::async, [params]() -> std::unique_ptr<AsyncStream> {
        // This would interface with actual LiteLLM library for streaming
        // For now, return a mock stream
        return std::make_unique<MockAsyncStream>();
    });
}

bool LitellmClient::is_available() {
    // This would check if LiteLLM is actually available
    // For now, assume it's available
    return true;
}

std::vector<std::string> LitellmClient::get_supported_providers() {
    std::vector<std::string> providers;
    for (const auto& [provider, models] : provider_models_) {
        providers.push_back(provider);
    }
    return providers;
}

std::vector<std::string> LitellmClient::get_supported_models(const std::string& provider) {
    auto it = provider_models_.find(provider);
    return it != provider_models_.end() ? it->second : std::vector<std::string>{};
}

// Factory function
std::shared_ptr<LitellmModel> create_litellm_model(
    const std::string& model,
    const std::optional<std::string>& base_url,
    const std::optional<std::string>& api_key
) {
    return std::make_shared<LitellmModel>(model, base_url, api_key);
}

// Utility functions
namespace utils {

std::pair<std::string, std::string> parse_model_string(const std::string& model) {
    // Parse provider:model format
    size_t colon_pos = model.find(':');
    if (colon_pos != std::string::npos) {
        return {model.substr(0, colon_pos), model.substr(colon_pos + 1)};
    }
    
    // Try to infer provider from model name
    if (model.find("gpt") != std::string::npos) {
        return {"openai", model};
    } else if (model.find("claude") != std::string::npos) {
        return {"anthropic", model};
    } else if (model.find("gemini") != std::string::npos) {
        return {"google", model};
    } else if (model.find("mistral") != std::string::npos) {
        return {"mistral", model};
    }
    
    return {"unknown", model};
}

bool validate_model_config(const std::string& model, const std::optional<std::string>& base_url) {
    auto [provider, model_name] = parse_model_string(model);
    
    if (provider == "unknown" && !base_url.has_value()) {
        return false;
    }
    
    return true;
}

std::optional<std::string> get_default_api_key(const std::string& provider) {
    // This would get API keys from environment variables
    if (provider == "openai") {
        return std::getenv("OPENAI_API_KEY");
    } else if (provider == "anthropic") {
        return std::getenv("ANTHROPIC_API_KEY");
    }
    return std::nullopt;
}

std::optional<std::string> get_default_base_url(const std::string& provider) {
    // Return default base URLs for known providers
    if (provider == "openai") {
        return "https://api.openai.com/v1";
    } else if (provider == "anthropic") {
        return "https://api.anthropic.com";
    }
    return std::nullopt;
}

} // namespace utils

} // namespace models
} // namespace extensions
} // namespace openai_agents