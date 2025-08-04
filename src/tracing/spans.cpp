#include "spans.h"
#include "processor_interface.h"
#include "scope.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace openai_agents {
namespace tracing {

template<typename TSpanData>
std::string SpanImpl<TSpanData>::current_time_iso() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

template<typename TSpanData>
SpanImpl<TSpanData>::SpanImpl(
    const std::string& trace_id,
    const std::string& span_id,
    const std::optional<std::string>& parent_id,
    const TSpanData& span_data,
    std::shared_ptr<TracingProcessor> processor
) : trace_id_(trace_id),
    span_id_(span_id),
    parent_id_(parent_id),
    span_data_(span_data),
    processor_(processor),
    is_current_(false) {}

template<typename TSpanData>
void SpanImpl<TSpanData>::start(bool mark_as_current) {
    started_at_ = current_time_iso();
    
    if (mark_as_current) {
        is_current_ = true;
        try {
            ScopedTracingContext::set_current_span_id(span_id_);
            ScopedTracingContext::set_current_trace_id(trace_id_);
        } catch (const std::exception& e) {
            logger::debug("Failed to set current span context: " + std::string(e.what()));
        }
    }
}

template<typename TSpanData>
void SpanImpl<TSpanData>::finish(bool reset_current) {
    ended_at_ = current_time_iso();
    
    if (reset_current || is_current_) {
        try {
            ScopedTracingContext::reset_current_span();
            ScopedTracingContext::reset_current_trace();
        } catch (const std::exception& e) {
            logger::debug("Failed to reset current span context: " + std::string(e.what()));
        }
        is_current_ = false;
    }
    
    // Send span to processor for processing
    if (processor_) {
        try {
            auto exported = export_span();
            if (exported) {
                processor_->process_span(*exported);
            }
        } catch (const std::exception& e) {
            logger::error("Failed to process span: " + std::string(e.what()));
        }
    }
}

template<typename TSpanData>
std::optional<nlohmann::json> SpanImpl<TSpanData>::export_span() const {
    try {
        nlohmann::json span_json;
        
        span_json["trace_id"] = trace_id_;
        span_json["span_id"] = span_id_;
        if (parent_id_) {
            span_json["parent_id"] = *parent_id_;
        }
        
        if (started_at_) {
            span_json["started_at"] = *started_at_;
        }
        if (ended_at_) {
            span_json["ended_at"] = *ended_at_;
        }
        
        // Export span data
        auto span_data_json = span_data_.to_json();
        span_json["span_data"] = span_data_json;
        
        // Add error if present
        if (error_) {
            nlohmann::json error_json;
            error_json["message"] = error_->message;
            if (error_->data) {
                nlohmann::json error_data_json;
                for (const auto& [key, value] : *error_->data) {
                    try {
                        // Try to convert std::any to JSON
                        if (value.type() == typeid(std::string)) {
                            error_data_json[key] = std::any_cast<std::string>(value);
                        } else if (value.type() == typeid(int)) {
                            error_data_json[key] = std::any_cast<int>(value);
                        } else if (value.type() == typeid(double)) {
                            error_data_json[key] = std::any_cast<double>(value);
                        } else if (value.type() == typeid(bool)) {
                            error_data_json[key] = std::any_cast<bool>(value);
                        } else {
                            // Fallback to string representation
                            error_data_json[key] = "unsupported_type";
                        }
                    } catch (const std::bad_any_cast& e) {
                        error_data_json[key] = "any_cast_error";
                    }
                }
                error_json["data"] = error_data_json;
            }
            span_json["error"] = error_json;
        }
        
        return span_json;
        
    } catch (const std::exception& e) {
        logger::error("Failed to export span: " + std::string(e.what()));
        return std::nullopt;
    }
}

// Explicit template instantiations for commonly used span types
template class SpanImpl<AgentSpanData>;
template class SpanImpl<FunctionSpanData>;
template class SpanImpl<GenerationSpanData>;
template class SpanImpl<ResponseSpanData>;
template class SpanImpl<HandoffSpanData>;
template class SpanImpl<CustomSpanData>;
template class SpanImpl<GuardrailSpanData>;
template class SpanImpl<TranscriptionSpanData>;
template class SpanImpl<SpeechSpanData>;
template class SpanImpl<SpeechGroupSpanData>;
template class SpanImpl<MCPListToolsSpanData>;

// AnySpan implementation
std::optional<nlohmann::json> AnySpan::export_span() const {
    try {
        nlohmann::json span_json;
        
        span_json["trace_id"] = trace_id_;
        span_json["span_id"] = span_id_;
        if (parent_id_) {
            span_json["parent_id"] = *parent_id_;
        }
        
        if (started_at_) {
            span_json["started_at"] = *started_at_;
        }
        if (ended_at_) {
            span_json["ended_at"] = *ended_at_;
        }
        
        // Export span data
        auto span_data_json = span_data_->to_json();
        span_json["span_data"] = span_data_json;
        
        // Add error if present
        if (error_) {
            nlohmann::json error_json;
            error_json["message"] = error_->message;
            if (error_->data) {
                nlohmann::json error_data_json;
                for (const auto& [key, value] : *error_->data) {
                    try {
                        // Try to convert std::any to JSON
                        if (value.type() == typeid(std::string)) {
                            error_data_json[key] = std::any_cast<std::string>(value);
                        } else if (value.type() == typeid(int)) {
                            error_data_json[key] = std::any_cast<int>(value);
                        } else if (value.type() == typeid(double)) {
                            error_data_json[key] = std::any_cast<double>(value);
                        } else if (value.type() == typeid(bool)) {
                            error_data_json[key] = std::any_cast<bool>(value);
                        } else {
                            // Fallback to string representation
                            error_data_json[key] = "unsupported_type";
                        }
                    } catch (const std::bad_any_cast& e) {
                        error_data_json[key] = "any_cast_error";
                    }
                }
                error_json["data"] = error_data_json;
            }
            span_json["error"] = error_json;
        }
        
        return span_json;
        
    } catch (const std::exception& e) {
        logger::error("Failed to export AnySpan: " + std::string(e.what()));
        return std::nullopt;
    }
}

} // namespace tracing
} // namespace openai_agents