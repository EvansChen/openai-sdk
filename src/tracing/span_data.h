#pragma once

/**
 * Span Data Types for OpenAI Agents Framework Tracing
 * 
 * This module defines various span data types that represent different
 * kinds of operations in the tracing system.
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <any>
#include <optional>
#include <nlohmann/json.hpp>

namespace openai_agents {
namespace tracing {

/**
 * Base class for all span data types
 */
class SpanData {
public:
    virtual ~SpanData() = default;
    
    /**
     * Export the span data as a JSON object
     */
    virtual nlohmann::json export_data() const = 0;
    
    /**
     * Return the type of the span
     */
    virtual std::string get_type() const = 0;
    
    /**
     * Clone the span data
     */
    virtual std::unique_ptr<SpanData> clone() const = 0;
};

/**
 * Agent span data - represents an agent execution
 */
class AgentSpanData : public SpanData {
public:
    std::string name;
    std::optional<std::vector<std::string>> handoffs;
    std::optional<std::vector<std::string>> tools;
    std::optional<std::string> output_type;
    
    AgentSpanData(
        const std::string& name,
        const std::optional<std::vector<std::string>>& handoffs = std::nullopt,
        const std::optional<std::vector<std::string>>& tools = std::nullopt,
        const std::optional<std::string>& output_type = std::nullopt
    );
    
    std::string get_type() const override { return "agent"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Function span data - represents a function call
 */
class FunctionSpanData : public SpanData {
public:
    std::string name;
    std::optional<std::string> input;
    std::any output;
    std::optional<nlohmann::json> mcp_data;
    
    FunctionSpanData(
        const std::string& name,
        const std::optional<std::string>& input = std::nullopt,
        const std::any& output = std::any{},
        const std::optional<nlohmann::json>& mcp_data = std::nullopt
    );
    
    std::string get_type() const override { return "function"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Generation span data - represents a model generation
 */
class GenerationSpanData : public SpanData {
public:
    std::optional<std::vector<nlohmann::json>> input;
    std::optional<std::vector<nlohmann::json>> output;
    std::optional<std::string> model;
    std::optional<nlohmann::json> model_config;
    std::optional<nlohmann::json> usage;
    
    GenerationSpanData(
        const std::optional<std::vector<nlohmann::json>>& input = std::nullopt,
        const std::optional<std::vector<nlohmann::json>>& output = std::nullopt,
        const std::optional<std::string>& model = std::nullopt,
        const std::optional<nlohmann::json>& model_config = std::nullopt,
        const std::optional<nlohmann::json>& usage = std::nullopt
    );
    
    std::string get_type() const override { return "generation"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Response span data - represents a response
 */
class ResponseSpanData : public SpanData {
public:
    std::optional<std::string> response_id;
    std::optional<std::any> input;
    
    ResponseSpanData(
        const std::optional<std::string>& response_id = std::nullopt,
        const std::optional<std::any>& input = std::nullopt
    );
    
    std::string get_type() const override { return "response"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Handoff span data - represents agent handoff
 */
class HandoffSpanData : public SpanData {
public:
    std::optional<std::string> from_agent;
    std::optional<std::string> to_agent;
    
    HandoffSpanData(
        const std::optional<std::string>& from_agent = std::nullopt,
        const std::optional<std::string>& to_agent = std::nullopt
    );
    
    std::string get_type() const override { return "handoff"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Custom span data - represents custom user-defined spans
 */
class CustomSpanData : public SpanData {
public:
    std::string name;
    nlohmann::json data;
    
    CustomSpanData(
        const std::string& name,
        const nlohmann::json& data = nlohmann::json::object()
    );
    
    std::string get_type() const override { return "custom"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Guardrail span data - represents guardrail execution
 */
class GuardrailSpanData : public SpanData {
public:
    std::string name;
    bool triggered;
    
    GuardrailSpanData(
        const std::string& name,
        bool triggered = false
    );
    
    std::string get_type() const override { return "guardrail"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Transcription span data - represents speech-to-text
 */
class TranscriptionSpanData : public SpanData {
public:
    std::optional<std::string> input;
    std::string input_format;
    std::optional<std::string> output;
    std::optional<std::string> model;
    std::optional<nlohmann::json> model_config;
    
    TranscriptionSpanData(
        const std::optional<std::string>& input = std::nullopt,
        const std::string& input_format = "pcm",
        const std::optional<std::string>& output = std::nullopt,
        const std::optional<std::string>& model = std::nullopt,
        const std::optional<nlohmann::json>& model_config = std::nullopt
    );
    
    std::string get_type() const override { return "transcription"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Speech span data - represents text-to-speech
 */
class SpeechSpanData : public SpanData {
public:
    std::optional<std::string> input;
    std::optional<std::string> output;
    std::string output_format;
    std::optional<std::string> model;
    std::optional<nlohmann::json> model_config;
    std::optional<std::string> first_content_at;
    
    SpeechSpanData(
        const std::optional<std::string>& input = std::nullopt,
        const std::optional<std::string>& output = std::nullopt,
        const std::string& output_format = "pcm",
        const std::optional<std::string>& model = std::nullopt,
        const std::optional<nlohmann::json>& model_config = std::nullopt,
        const std::optional<std::string>& first_content_at = std::nullopt
    );
    
    std::string get_type() const override { return "speech"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * Speech group span data - represents a group of speech operations
 */
class SpeechGroupSpanData : public SpanData {
public:
    std::optional<std::string> input;
    
    explicit SpeechGroupSpanData(
        const std::optional<std::string>& input = std::nullopt
    );
    
    std::string get_type() const override { return "speech_group"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

/**
 * MCP list tools span data - represents MCP tool listing
 */
class MCPListToolsSpanData : public SpanData {
public:
    std::optional<std::string> server;
    std::optional<std::vector<std::string>> result;
    
    MCPListToolsSpanData(
        const std::optional<std::string>& server = std::nullopt,
        const std::optional<std::vector<std::string>>& result = std::nullopt
    );
    
    std::string get_type() const override { return "mcp_tools"; }
    
    nlohmann::json export_data() const override;
    
    std::unique_ptr<SpanData> clone() const override;
};

} // namespace tracing
} // namespace openai_agents