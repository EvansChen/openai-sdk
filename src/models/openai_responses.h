#pragma once

/**
 * OpenAI responses model implementation
 */

#include "interface.h"
#include "../usage.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>
#include <any>

namespace openai_agents {
namespace models {

// Response structures
struct ChatMessage {
    std::string role;
    std::string content;
    std::optional<std::string> name;
    std::map<std::string, std::any> metadata;
};

struct ToolCall {
    std::string id;
    std::string type;
    std::string function_name;
    std::string arguments;
};

struct ChatChoice {
    size_t index;
    ChatMessage message;
    std::vector<ToolCall> tool_calls;
    std::string finish_reason;
};

struct ChatCompletionResponse {
    std::string id;
    std::string object;
    int64_t created;
    std::string model;
    std::vector<ChatChoice> choices;
    Usage usage;
    std::map<std::string, std::any> metadata;
};

// Streaming response
struct StreamingChunk {
    std::string id;
    std::string object;
    int64_t created;
    std::string model;
    std::vector<ChatChoice> choices;
    bool is_complete;
};

class OpenAIResponsesModel : public Model {
private:
    std::string model_name_;
    std::string api_key_;
    std::string base_url_;
    std::map<std::string, std::string> default_headers_;
    int timeout_seconds_;

public:
    OpenAIResponsesModel(const std::string& model_name, const std::string& api_key,
                        const std::string& base_url = "https://api.openai.com/v1");
    
    // Model interface implementation
    std::string get_name() const override;
    std::string generate(const std::string& prompt) override;
    
    // Advanced generation methods
    ChatCompletionResponse chat_completion(
        const std::vector<ChatMessage>& messages,
        const std::map<std::string, std::any>& options = {}
    );
    
    std::vector<StreamingChunk> stream_chat_completion(
        const std::vector<ChatMessage>& messages,
        const std::map<std::string, std::any>& options = {}
    );
    
    // Tool support
    ChatCompletionResponse chat_completion_with_tools(
        const std::vector<ChatMessage>& messages,
        const std::vector<std::map<std::string, std::any>>& tools,
        const std::map<std::string, std::any>& options = {}
    );
    
    // Configuration
    void set_api_key(const std::string& api_key) { api_key_ = api_key; }
    void set_base_url(const std::string& base_url) { base_url_ = base_url; }
    void set_timeout(int seconds) { timeout_seconds_ = seconds; }
    void add_default_header(const std::string& key, const std::string& value);
    
    // Getters
    const std::string& get_api_key() const { return api_key_; }
    const std::string& get_base_url() const { return base_url_; }
    int get_timeout() const { return timeout_seconds_; }

private:
    // HTTP client methods
    std::string make_request(const std::string& endpoint, const std::string& json_data);
    std::map<std::string, std::string> prepare_headers() const;
    std::string build_chat_request_json(
        const std::vector<ChatMessage>& messages,
        const std::map<std::string, std::any>& options
    ) const;
    
    // Response parsing
    ChatCompletionResponse parse_chat_response(const std::string& json_response) const;
    StreamingChunk parse_streaming_chunk(const std::string& json_chunk) const;
    
    // Validation
    void validate_messages(const std::vector<ChatMessage>& messages) const;
    void validate_tools(const std::vector<std::map<std::string, std::any>>& tools) const;
};

// Helper functions
ChatMessage create_user_message(const std::string& content);
ChatMessage create_assistant_message(const std::string& content);
ChatMessage create_system_message(const std::string& content);
ChatMessage create_tool_message(const std::string& tool_call_id, const std::string& content);

// Response utilities
std::string extract_content_from_response(const ChatCompletionResponse& response);
std::vector<ToolCall> extract_tool_calls_from_response(const ChatCompletionResponse& response);
bool is_response_complete(const ChatCompletionResponse& response);

} // namespace models
} // namespace openai_agents