#pragma once

/**
 * Span Creation Utilities for OpenAI Agents Framework Tracing
 * 
 * This module provides convenient factory functions for creating spans
 * with proper initialization and context management.
 */

#include "spans.h"
#include "traces.h"
#include "scope.h"
#include "processor_interface.h"
#include <memory>
#include <functional>

namespace openai_agents {
namespace tracing {

/**
 * Span creation options
 */
struct SpanCreationOptions {
    std::optional<std::string> trace_id;
    std::optional<std::string> parent_span_id;
    bool auto_start = true;
    bool mark_as_current = false;
    std::shared_ptr<TracingProcessor> processor;
    
    SpanCreationOptions() = default;
};

/**
 * Span factory for creating different types of spans
 */
class SpanFactory {
private:
    std::shared_ptr<TracingProcessor> default_processor_;
    
public:
    /**
     * Create a span factory with a default processor
     */
    explicit SpanFactory(std::shared_ptr<TracingProcessor> processor = nullptr)
        : default_processor_(processor) {}
    
    /**
     * Create an agent span
     */
    std::unique_ptr<AgentSpan> create_agent_span(
        const AgentSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a function span
     */
    std::unique_ptr<FunctionSpan> create_function_span(
        const FunctionSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a generation span
     */
    std::unique_ptr<GenerationSpan> create_generation_span(
        const GenerationSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a response span
     */
    std::unique_ptr<ResponseSpan> create_response_span(
        const ResponseSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a handoff span
     */
    std::unique_ptr<HandoffSpan> create_handoff_span(
        const HandoffSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a custom span
     */
    std::unique_ptr<CustomSpan> create_custom_span(
        const CustomSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a guardrail span
     */
    std::unique_ptr<GuardrailSpan> create_guardrail_span(
        const GuardrailSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a transcription span
     */
    std::unique_ptr<TranscriptionSpan> create_transcription_span(
        const TranscriptionSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a speech span
     */
    std::unique_ptr<SpeechSpan> create_speech_span(
        const SpeechSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create a speech group span
     */
    std::unique_ptr<SpeechGroupSpan> create_speech_group_span(
        const SpeechGroupSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Create an MCP list tools span
     */
    std::unique_ptr<MCPListToolsSpan> create_mcp_list_tools_span(
        const MCPListToolsSpanData& span_data,
        const SpanCreationOptions& options = {}
    );
    
    /**
     * Set the default processor
     */
    void set_default_processor(std::shared_ptr<TracingProcessor> processor) {
        default_processor_ = processor;
    }
    
    /**
     * Get the default processor
     */
    std::shared_ptr<TracingProcessor> get_default_processor() const {
        return default_processor_;
    }
    
private:
    /**
     * Generic span creation helper
     */
    template<typename TSpanData>
    std::unique_ptr<Span<TSpanData>> create_span_impl(
        const TSpanData& span_data,
        const SpanCreationOptions& options
    );
    
    /**
     * Resolve the trace ID for a new span
     */
    std::string resolve_trace_id(const SpanCreationOptions& options);
    
    /**
     * Resolve the parent span ID
     */
    std::optional<std::string> resolve_parent_span_id(const SpanCreationOptions& options);
    
    /**
     * Resolve the processor to use
     */
    std::shared_ptr<TracingProcessor> resolve_processor(const SpanCreationOptions& options);
};

/**
 * Global span factory instance
 */
class GlobalSpanFactory {
private:
    static std::unique_ptr<SpanFactory> instance_;
    static std::mutex instance_mutex_;
    
public:
    /**
     * Get the global span factory instance
     */
    static SpanFactory& instance();
    
    /**
     * Initialize the global span factory
     */
    static void initialize(std::shared_ptr<TracingProcessor> processor = nullptr);
    
    /**
     * Shutdown the global span factory
     */
    static void shutdown();
    
    /**
     * Check if the global span factory is initialized
     */
    static bool is_initialized();
};

/**
 * Convenience functions for creating spans
 */
namespace create {

/**
 * Create an agent span
 */
std::unique_ptr<AgentSpan> agent_span(
    const std::string& agent_name,
    const SpanCreationOptions& options = {}
);

/**
 * Create a function span
 */
std::unique_ptr<FunctionSpan> function_span(
    const std::string& function_name,
    const nlohmann::json& arguments = nlohmann::json::object(),
    const SpanCreationOptions& options = {}
);

/**
 * Create a generation span
 */
std::unique_ptr<GenerationSpan> generation_span(
    const nlohmann::json& messages,
    const SpanCreationOptions& options = {}
);

/**
 * Create a response span
 */
std::unique_ptr<ResponseSpan> response_span(
    const nlohmann::json& response,
    const SpanCreationOptions& options = {}
);

/**
 * Create a handoff span
 */
std::unique_ptr<HandoffSpan> handoff_span(
    const std::string& target,
    const std::optional<nlohmann::json>& message = std::nullopt,
    const SpanCreationOptions& options = {}
);

/**
 * Create a custom span
 */
std::unique_ptr<CustomSpan> custom_span(
    const std::string& name,
    const std::unordered_map<std::string, std::any>& data = {},
    const SpanCreationOptions& options = {}
);

/**
 * Create a guardrail span
 */
std::unique_ptr<GuardrailSpan> guardrail_span(
    const std::string& guardrail_name,
    const std::optional<nlohmann::json>& result = std::nullopt,
    const SpanCreationOptions& options = {}
);

/**
 * Create a transcription span
 */
std::unique_ptr<TranscriptionSpan> transcription_span(
    const std::string& model,
    const std::optional<std::string>& transcript = std::nullopt,
    const SpanCreationOptions& options = {}
);

/**
 * Create a speech span
 */
std::unique_ptr<SpeechSpan> speech_span(
    const std::string& model,
    const std::string& voice,
    const std::string& input,
    const SpanCreationOptions& options = {}
);

/**
 * Create a speech group span
 */
std::unique_ptr<SpeechGroupSpan> speech_group_span(
    const std::vector<std::string>& text_inputs,
    const SpanCreationOptions& options = {}
);

/**
 * Create an MCP list tools span
 */
std::unique_ptr<MCPListToolsSpan> mcp_list_tools_span(
    const std::string& server_name,
    const std::optional<std::vector<nlohmann::json>>& tools = std::nullopt,
    const SpanCreationOptions& options = {}
);

} // namespace create

/**
 * Higher-level span creation with automatic context management
 */
namespace auto_span {

/**
 * Create and start an agent span with automatic finishing
 */
template<typename Func>
auto agent(const std::string& agent_name, Func&& func, 
           const SpanCreationOptions& options = {}) -> decltype(func()) {
    auto span = create::agent_span(agent_name, options);
    auto guard = make_span_guard(std::move(span));
    return func();
}

/**
 * Create and start a function span with automatic finishing
 */
template<typename Func>
auto function(const std::string& function_name, Func&& func,
              const nlohmann::json& arguments = nlohmann::json::object(),
              const SpanCreationOptions& options = {}) -> decltype(func()) {
    auto span = create::function_span(function_name, arguments, options);
    auto guard = make_span_guard(std::move(span));
    try {
        return func();
    } catch (const std::exception& e) {
        guard->set_error(SpanError(e.what()));
        throw;
    }
}

/**
 * Create and start a generation span with automatic finishing
 */
template<typename Func>
auto generation(const nlohmann::json& messages, Func&& func,
                const SpanCreationOptions& options = {}) -> decltype(func()) {
    auto span = create::generation_span(messages, options);
    auto guard = make_span_guard(std::move(span));
    try {
        return func();
    } catch (const std::exception& e) {
        guard->set_error(SpanError(e.what()));
        throw;
    }
}

/**
 * Create and start a custom span with automatic finishing
 */
template<typename Func>
auto custom(const std::string& name, Func&& func,
            const std::unordered_map<std::string, std::any>& data = {},
            const SpanCreationOptions& options = {}) -> decltype(func()) {
    auto span = create::custom_span(name, data, options);
    auto guard = make_span_guard(std::move(span));
    try {
        return func();
    } catch (const std::exception& e) {
        guard->set_error(SpanError(e.what()));
        throw;
    }
}

/**
 * Create and start a guardrail span with automatic finishing
 */
template<typename Func>
auto guardrail(const std::string& guardrail_name, Func&& func,
               const SpanCreationOptions& options = {}) -> decltype(func()) {
    auto span = create::guardrail_span(guardrail_name, std::nullopt, options);
    auto guard = make_span_guard(std::move(span));
    try {
        auto result = func();
        // Capture result if possible
        // This would need template specialization for different return types
        return result;
    } catch (const std::exception& e) {
        guard->set_error(SpanError(e.what()));
        throw;
    }
}

} // namespace auto_span

/**
 * Tracing decorators for automatic span creation
 */
namespace decorators {

/**
 * Function wrapper that automatically creates spans
 */
template<typename Func>
class TracedFunction {
private:
    Func func_;
    std::string name_;
    SpanCreationOptions options_;
    
public:
    TracedFunction(Func func, const std::string& name, const SpanCreationOptions& options = {})
        : func_(std::move(func)), name_(name), options_(options) {}
    
    template<typename... Args>
    auto operator()(Args&&... args) -> decltype(func_(std::forward<Args>(args)...)) {
        return auto_span::function(name_, [&]() {
            return func_(std::forward<Args>(args)...);
        }, nlohmann::json::object(), options_);
    }
};

/**
 * Create a traced function wrapper
 */
template<typename Func>
TracedFunction<Func> trace_function(Func func, const std::string& name, 
                                   const SpanCreationOptions& options = {}) {
    return TracedFunction<Func>(std::move(func), name, options);
}

/**
 * Macro for easy function tracing
 */
#define TRACE_FUNCTION(func_name) \
    decorators::trace_function([&](auto&&... args) { \
        return func_name(std::forward<decltype(args)>(args)...); \
    }, #func_name)

} // namespace decorators

} // namespace tracing
} // namespace openai_agents