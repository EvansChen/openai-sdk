#include "openai_responses.h"
#include "../exceptions.h"
#include "../logger.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <random>

namespace openai_agents {
namespace models {

OpenAIResponsesModel::OpenAIResponsesModel(const std::string& model_name, const std::string& api_key,
                                         const std::string& base_url)
    : model_name_(model_name), api_key_(api_key), base_url_(base_url), timeout_seconds_(30) {
    if (model_name_.empty()) {
        throw AgentsException("Model name cannot be empty");
    }
    if (api_key_.empty()) {
        throw AgentsException("API key cannot be empty");
    }
    
    // Set default headers
    default_headers_["Content-Type"] = "application/json";
    default_headers_["Authorization"] = "Bearer " + api_key_;
}

std::string OpenAIResponsesModel::get_name() const {
    return model_name_;
}

std::string OpenAIResponsesModel::generate(const std::string& prompt) {
    if (prompt.empty()) {
        return "";
    }

    // Convert prompt to chat message and use chat completion
    std::vector<ChatMessage> messages = {create_user_message(prompt)};
    auto response = chat_completion(messages);
    
    return extract_content_from_response(response);
}

ChatCompletionResponse OpenAIResponsesModel::chat_completion(
    const std::vector<ChatMessage>& messages,
    const std::map<std::string, std::any>& options
) {
    validate_messages(messages);
    
    std::string json_request = build_chat_request_json(messages, options);
    std::string json_response = make_request("/chat/completions", json_request);
    
    return parse_chat_response(json_response);
}

std::vector<StreamingChunk> OpenAIResponsesModel::stream_chat_completion(
    const std::vector<ChatMessage>& messages,
    const std::map<std::string, std::any>& options
) {
    validate_messages(messages);
    
    // Add streaming parameter
    auto streaming_options = options;
    streaming_options["stream"] = true;
    
    std::string json_request = build_chat_request_json(messages, streaming_options);
    std::string json_response = make_request("/chat/completions", json_request);
    
    // In a real implementation, this would handle Server-Sent Events
    // For now, return a single chunk
    std::vector<StreamingChunk> chunks;
    StreamingChunk chunk = parse_streaming_chunk(json_response);
    chunks.push_back(chunk);
    
    return chunks;
}

ChatCompletionResponse OpenAIResponsesModel::chat_completion_with_tools(
    const std::vector<ChatMessage>& messages,
    const std::vector<std::map<std::string, std::any>>& tools,
    const std::map<std::string, std::any>& options
) {
    validate_messages(messages);
    validate_tools(tools);
    
    auto tool_options = options;
    tool_options["tools"] = tools;
    
    return chat_completion(messages, tool_options);
}

void OpenAIResponsesModel::add_default_header(const std::string& key, const std::string& value) {
    default_headers_[key] = value;
}

std::string OpenAIResponsesModel::make_request(const std::string& endpoint, const std::string& json_data) {
    // In a real implementation, this would use an HTTP client library
    // For now, return a mock response
    
    auto logger = get_logger("OpenAIResponsesModel");
    logger->info("Making request to: " + base_url_ + endpoint);
    logger->debug("Request data: " + json_data);
    
    // Simulate API delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Mock response
    std::ostringstream mock_response;
    mock_response << "{"
                  << "\"id\": \"chatcmpl-mock123\","
                  << "\"object\": \"chat.completion\","
                  << "\"created\": " << std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch()).count() << ","
                  << "\"model\": \"" << model_name_ << "\","
                  << "\"choices\": [{"
                  << "\"index\": 0,"
                  << "\"message\": {"
                  << "\"role\": \"assistant\","
                  << "\"content\": \"This is a mock response from " << model_name_ << "\""
                  << "},"
                  << "\"finish_reason\": \"stop\""
                  << "}],"
                  << "\"usage\": {"
                  << "\"prompt_tokens\": 10,"
                  << "\"completion_tokens\": 15,"
                  << "\"total_tokens\": 25"
                  << "}"
                  << "}";
    
    return mock_response.str();
}

std::map<std::string, std::string> OpenAIResponsesModel::prepare_headers() const {
    auto headers = default_headers_;
    headers["User-Agent"] = "OpenAI-CPP-Agent/1.0";
    return headers;
}

std::string OpenAIResponsesModel::build_chat_request_json(
    const std::vector<ChatMessage>& messages,
    const std::map<std::string, std::any>& options
) const {
    std::ostringstream json;
    json << "{";
    json << "\"model\": \"" << model_name_ << "\",";
    json << "\"messages\": [";
    
    for (size_t i = 0; i < messages.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"role\": \"" << messages[i].role << "\",";
        json << "\"content\": \"" << messages[i].content << "\"";
        if (messages[i].name) {
            json << ",\"name\": \"" << *messages[i].name << "\"";
        }
        json << "}";
    }
    
    json << "]";
    
    // Add options
    for (const auto& [key, value] : options) {
        json << ",\"" << key << "\": ";
        // Simplified serialization - in real implementation would use proper JSON library
        try {
            json << std::any_cast<std::string>(value);
        } catch (const std::bad_any_cast&) {
            try {
                json << std::any_cast<int>(value);
            } catch (const std::bad_any_cast&) {
                try {
                    json << std::any_cast<double>(value);
                } catch (const std::bad_any_cast&) {
                    json << "null";
                }
            }
        }
    }
    
    json << "}";
    return json.str();
}

ChatCompletionResponse OpenAIResponsesModel::parse_chat_response(const std::string& json_response) const {
    // In a real implementation, this would use a JSON parser
    // For now, create a mock response
    ChatCompletionResponse response;
    response.id = "chatcmpl-mock123";
    response.object = "chat.completion";
    response.created = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    response.model = model_name_;
    
    ChatChoice choice;
    choice.index = 0;
    choice.message.role = "assistant";
    choice.message.content = "This is a mock response from " + model_name_;
    choice.finish_reason = "stop";
    response.choices.push_back(choice);
    
    response.usage.prompt_tokens = 10;
    response.usage.completion_tokens = 15;
    response.usage.total_tokens = 25;
    
    return response;
}

StreamingChunk OpenAIResponsesModel::parse_streaming_chunk(const std::string& json_chunk) const {
    // In a real implementation, this would parse SSE data
    StreamingChunk chunk;
    chunk.id = "chatcmpl-mock123";
    chunk.object = "chat.completion.chunk";
    chunk.created = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    chunk.model = model_name_;
    chunk.is_complete = true;
    
    ChatChoice choice;
    choice.index = 0;
    choice.message.role = "assistant";
    choice.message.content = "Streaming response from " + model_name_;
    choice.finish_reason = "stop";
    chunk.choices.push_back(choice);
    
    return chunk;
}

void OpenAIResponsesModel::validate_messages(const std::vector<ChatMessage>& messages) const {
    if (messages.empty()) {
        throw AgentsException("Messages cannot be empty");
    }
    
    for (const auto& message : messages) {
        if (message.role.empty()) {
            throw AgentsException("Message role cannot be empty");
        }
        if (message.content.empty()) {
            throw AgentsException("Message content cannot be empty");
        }
    }
}

void OpenAIResponsesModel::validate_tools(const std::vector<std::map<std::string, std::any>>& tools) const {
    for (const auto& tool : tools) {
        if (tool.find("type") == tool.end()) {
            throw AgentsException("Tool must have a type");
        }
        if (tool.find("function") == tool.end()) {
            throw AgentsException("Tool must have a function definition");
        }
    }
}

// Helper functions
ChatMessage create_user_message(const std::string& content) {
    ChatMessage message;
    message.role = "user";
    message.content = content;
    return message;
}

ChatMessage create_assistant_message(const std::string& content) {
    ChatMessage message;
    message.role = "assistant";
    message.content = content;
    return message;
}

ChatMessage create_system_message(const std::string& content) {
    ChatMessage message;
    message.role = "system";
    message.content = content;
    return message;
}

ChatMessage create_tool_message(const std::string& tool_call_id, const std::string& content) {
    ChatMessage message;
    message.role = "tool";
    message.content = content;
    message.metadata["tool_call_id"] = tool_call_id;
    return message;
}

// Response utilities
std::string extract_content_from_response(const ChatCompletionResponse& response) {
    if (!response.choices.empty()) {
        return response.choices[0].message.content;
    }
    return "";
}

std::vector<ToolCall> extract_tool_calls_from_response(const ChatCompletionResponse& response) {
    if (!response.choices.empty()) {
        return response.choices[0].tool_calls;
    }
    return {};
}

bool is_response_complete(const ChatCompletionResponse& response) {
    if (!response.choices.empty()) {
        return response.choices[0].finish_reason == "stop" || 
               response.choices[0].finish_reason == "tool_calls";
    }
    return false;
}

} // namespace models
} // namespace openai_agents