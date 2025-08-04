#include "create.h"
#include "util.h"

namespace openai_agents {
namespace tracing {

// SpanFactory implementation
template<typename TSpanData>
std::unique_ptr<Span<TSpanData>> SpanFactory::create_span_impl(
    const TSpanData& span_data,
    const SpanCreationOptions& options
) {
    auto trace_id = resolve_trace_id(options);
    auto parent_span_id = resolve_parent_span_id(options);
    auto processor = resolve_processor(options);
    auto span_id = trace_utils::generate_span_id();
    
    // Check if tracing is disabled
    if (ScopedTracingContext::is_trace_disabled()) {
        auto no_op_span = std::make_unique<NoOpSpan<TSpanData>>(span_data);
        if (options.auto_start) {
            no_op_span->start(options.mark_as_current);
        }
        return std::move(no_op_span);
    }
    
    // Create real span
    auto span = std::make_unique<SpanImpl<TSpanData>>(
        trace_id, span_id, parent_span_id, span_data, processor
    );
    
    if (options.auto_start) {
        span->start(options.mark_as_current);
    }
    
    // Add span to trace if trace manager is available
    try {
        if (GlobalTraceManager::is_initialized()) {
            GlobalTraceManager::instance().add_span_to_trace(trace_id, *span);
        }
    } catch (const std::exception& e) {
        logger::debug("Failed to add span to trace: " + std::string(e.what()));
    }
    
    return std::move(span);
}

std::string SpanFactory::resolve_trace_id(const SpanCreationOptions& options) {
    if (options.trace_id) {
        return *options.trace_id;
    }
    
    auto current_trace_id = ScopedTracingContext::get_current_trace_id();
    if (current_trace_id) {
        return *current_trace_id;
    }
    
    // Create a new trace if none exists
    try {
        if (GlobalTraceManager::is_initialized()) {
            return GlobalTraceManager::instance().start_current_trace();
        }
    } catch (const std::exception& e) {
        logger::debug("Failed to create new trace: " + std::string(e.what()));
    }
    
    return trace_utils::generate_trace_id();
}

std::optional<std::string> SpanFactory::resolve_parent_span_id(const SpanCreationOptions& options) {
    if (options.parent_span_id) {
        return options.parent_span_id;
    }
    
    return ScopedTracingContext::get_current_span_id();
}

std::shared_ptr<TracingProcessor> SpanFactory::resolve_processor(const SpanCreationOptions& options) {
    if (options.processor) {
        return options.processor;
    }
    return default_processor_;
}

std::unique_ptr<AgentSpan> SpanFactory::create_agent_span(
    const AgentSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<FunctionSpan> SpanFactory::create_function_span(
    const FunctionSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<GenerationSpan> SpanFactory::create_generation_span(
    const GenerationSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<ResponseSpan> SpanFactory::create_response_span(
    const ResponseSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<HandoffSpan> SpanFactory::create_handoff_span(
    const HandoffSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<CustomSpan> SpanFactory::create_custom_span(
    const CustomSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<GuardrailSpan> SpanFactory::create_guardrail_span(
    const GuardrailSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<TranscriptionSpan> SpanFactory::create_transcription_span(
    const TranscriptionSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<SpeechSpan> SpanFactory::create_speech_span(
    const SpeechSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<SpeechGroupSpan> SpanFactory::create_speech_group_span(
    const SpeechGroupSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

std::unique_ptr<MCPListToolsSpan> SpanFactory::create_mcp_list_tools_span(
    const MCPListToolsSpanData& span_data,
    const SpanCreationOptions& options
) {
    return create_span_impl(span_data, options);
}

// Explicit template instantiation
template std::unique_ptr<Span<AgentSpanData>> SpanFactory::create_span_impl(
    const AgentSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<FunctionSpanData>> SpanFactory::create_span_impl(
    const FunctionSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<GenerationSpanData>> SpanFactory::create_span_impl(
    const GenerationSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<ResponseSpanData>> SpanFactory::create_span_impl(
    const ResponseSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<HandoffSpanData>> SpanFactory::create_span_impl(
    const HandoffSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<CustomSpanData>> SpanFactory::create_span_impl(
    const CustomSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<GuardrailSpanData>> SpanFactory::create_span_impl(
    const GuardrailSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<TranscriptionSpanData>> SpanFactory::create_span_impl(
    const TranscriptionSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<SpeechSpanData>> SpanFactory::create_span_impl(
    const SpeechSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<SpeechGroupSpanData>> SpanFactory::create_span_impl(
    const SpeechGroupSpanData& span_data, const SpanCreationOptions& options);
template std::unique_ptr<Span<MCPListToolsSpanData>> SpanFactory::create_span_impl(
    const MCPListToolsSpanData& span_data, const SpanCreationOptions& options);

// GlobalSpanFactory implementation
std::unique_ptr<SpanFactory> GlobalSpanFactory::instance_;
std::mutex GlobalSpanFactory::instance_mutex_;

SpanFactory& GlobalSpanFactory::instance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        instance_ = std::make_unique<SpanFactory>();
    }
    return *instance_;
}

void GlobalSpanFactory::initialize(std::shared_ptr<TracingProcessor> processor) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_ = std::make_unique<SpanFactory>(processor);
}

void GlobalSpanFactory::shutdown() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_.reset();
}

bool GlobalSpanFactory::is_initialized() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    return instance_ != nullptr;
}

// Convenience functions implementation
namespace create {

std::unique_ptr<AgentSpan> agent_span(
    const std::string& agent_name,
    const SpanCreationOptions& options
) {
    AgentSpanData span_data;
    span_data.agent_name = agent_name;
    return GlobalSpanFactory::instance().create_agent_span(span_data, options);
}

std::unique_ptr<FunctionSpan> function_span(
    const std::string& function_name,
    const nlohmann::json& arguments,
    const SpanCreationOptions& options
) {
    FunctionSpanData span_data;
    span_data.function_name = function_name;
    span_data.arguments = arguments;
    return GlobalSpanFactory::instance().create_function_span(span_data, options);
}

std::unique_ptr<GenerationSpan> generation_span(
    const nlohmann::json& messages,
    const SpanCreationOptions& options
) {
    GenerationSpanData span_data;
    span_data.messages = messages;
    return GlobalSpanFactory::instance().create_generation_span(span_data, options);
}

std::unique_ptr<ResponseSpan> response_span(
    const nlohmann::json& response,
    const SpanCreationOptions& options
) {
    ResponseSpanData span_data;
    span_data.response = response;
    return GlobalSpanFactory::instance().create_response_span(span_data, options);
}

std::unique_ptr<HandoffSpan> handoff_span(
    const std::string& target,
    const std::optional<nlohmann::json>& message,
    const SpanCreationOptions& options
) {
    HandoffSpanData span_data;
    span_data.target = target;
    span_data.message = message;
    return GlobalSpanFactory::instance().create_handoff_span(span_data, options);
}

std::unique_ptr<CustomSpan> custom_span(
    const std::string& name,
    const std::unordered_map<std::string, std::any>& data,
    const SpanCreationOptions& options
) {
    CustomSpanData span_data;
    span_data.name = name;
    span_data.data = data;
    return GlobalSpanFactory::instance().create_custom_span(span_data, options);
}

std::unique_ptr<GuardrailSpan> guardrail_span(
    const std::string& guardrail_name,
    const std::optional<nlohmann::json>& result,
    const SpanCreationOptions& options
) {
    GuardrailSpanData span_data;
    span_data.guardrail_name = guardrail_name;
    span_data.result = result;
    return GlobalSpanFactory::instance().create_guardrail_span(span_data, options);
}

std::unique_ptr<TranscriptionSpan> transcription_span(
    const std::string& model,
    const std::optional<std::string>& transcript,
    const SpanCreationOptions& options
) {
    TranscriptionSpanData span_data;
    span_data.model = model;
    span_data.transcript = transcript;
    return GlobalSpanFactory::instance().create_transcription_span(span_data, options);
}

std::unique_ptr<SpeechSpan> speech_span(
    const std::string& model,
    const std::string& voice,
    const std::string& input,
    const SpanCreationOptions& options
) {
    SpeechSpanData span_data;
    span_data.model = model;
    span_data.voice = voice;
    span_data.input = input;
    return GlobalSpanFactory::instance().create_speech_span(span_data, options);
}

std::unique_ptr<SpeechGroupSpan> speech_group_span(
    const std::vector<std::string>& text_inputs,
    const SpanCreationOptions& options
) {
    SpeechGroupSpanData span_data;
    span_data.text_inputs = text_inputs;
    return GlobalSpanFactory::instance().create_speech_group_span(span_data, options);
}

std::unique_ptr<MCPListToolsSpan> mcp_list_tools_span(
    const std::string& server_name,
    const std::optional<std::vector<nlohmann::json>>& tools,
    const SpanCreationOptions& options
) {
    MCPListToolsSpanData span_data;
    span_data.server_name = server_name;
    span_data.tools = tools;
    return GlobalSpanFactory::instance().create_mcp_list_tools_span(span_data, options);
}

} // namespace create

} // namespace tracing
} // namespace openai_agents