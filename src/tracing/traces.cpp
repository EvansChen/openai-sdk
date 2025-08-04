#include "traces.h"
#include "processor_interface.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

namespace openai_agents {
namespace tracing {

// Trace implementation
std::string Trace::current_time_iso() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

Trace::Trace(const std::string& trace_id) : trace_id_(trace_id) {
    started_at_ = current_time_iso();
}

std::vector<const AnySpan*> Trace::get_spans() const {
    std::lock_guard<std::mutex> lock(spans_mutex_);
    std::vector<const AnySpan*> result;
    for (const auto& span : spans_) {
        result.push_back(span.get());
    }
    return result;
}

const AnySpan* Trace::get_span_by_id(const std::string& span_id) const {
    std::lock_guard<std::mutex> lock(spans_mutex_);
    for (const auto& span : spans_) {
        if (span->get_span_id() == span_id) {
            return span.get();
        }
    }
    return nullptr;
}

std::vector<const AnySpan*> Trace::get_root_spans() const {
    std::lock_guard<std::mutex> lock(spans_mutex_);
    std::vector<const AnySpan*> result;
    for (const auto& span : spans_) {
        if (!span->get_parent_id()) {
            result.push_back(span.get());
        }
    }
    return result;
}

std::vector<const AnySpan*> Trace::get_child_spans(const std::string& parent_span_id) const {
    std::lock_guard<std::mutex> lock(spans_mutex_);
    std::vector<const AnySpan*> result;
    for (const auto& span : spans_) {
        if (span->get_parent_id() && *span->get_parent_id() == parent_span_id) {
            result.push_back(span.get());
        }
    }
    return result;
}

void Trace::finish() {
    if (!is_finished_.exchange(true)) {
        ended_at_ = current_time_iso();
    }
}

std::optional<nlohmann::json> Trace::export_trace() const {
    try {
        nlohmann::json trace_json;
        
        trace_json["trace_id"] = trace_id_;
        if (started_at_) {
            trace_json["started_at"] = *started_at_;
        }
        if (ended_at_) {
            trace_json["ended_at"] = *ended_at_;
        }
        trace_json["is_finished"] = is_finished_.load();
        
        // Export metadata
        if (!metadata_.empty()) {
            nlohmann::json metadata_json;
            for (const auto& [key, value] : metadata_) {
                try {
                    if (value.type() == typeid(std::string)) {
                        metadata_json[key] = std::any_cast<std::string>(value);
                    } else if (value.type() == typeid(int)) {
                        metadata_json[key] = std::any_cast<int>(value);
                    } else if (value.type() == typeid(double)) {
                        metadata_json[key] = std::any_cast<double>(value);
                    } else if (value.type() == typeid(bool)) {
                        metadata_json[key] = std::any_cast<bool>(value);
                    } else {
                        metadata_json[key] = "unsupported_type";
                    }
                } catch (const std::bad_any_cast&) {
                    metadata_json[key] = "any_cast_error";
                }
            }
            trace_json["metadata"] = metadata_json;
        }
        
        // Export spans
        std::lock_guard<std::mutex> lock(spans_mutex_);
        nlohmann::json spans_json = nlohmann::json::array();
        for (const auto& span : spans_) {
            auto span_json = span->export_span();
            if (span_json) {
                spans_json.push_back(*span_json);
            }
        }
        trace_json["spans"] = spans_json;
        
        // Add statistics
        auto stats = get_stats();
        nlohmann::json stats_json;
        stats_json["total_spans"] = stats.total_spans;
        stats_json["error_spans"] = stats.error_spans;
        if (stats.duration) {
            stats_json["duration_ms"] = stats.duration->count();
        }
        stats_json["span_types"] = stats.span_types;
        trace_json["stats"] = stats_json;
        
        return trace_json;
        
    } catch (const std::exception& e) {
        logger::error("Failed to export trace: " + std::string(e.what()));
        return std::nullopt;
    }
}

Trace::TraceStats Trace::get_stats() const {
    std::lock_guard<std::mutex> lock(spans_mutex_);
    TraceStats stats;
    
    stats.total_spans = spans_.size();
    
    for (const auto& span : spans_) {
        // Count error spans
        if (span->get_error()) {
            stats.error_spans++;
        }
        
        // Count span types
        auto span_type = span->get_span_data().get_type();
        stats.span_types[span_type]++;
    }
    
    // Calculate duration
    stats.duration = get_duration();
    
    return stats;
}

std::optional<std::chrono::milliseconds> Trace::get_duration() const {
    if (started_at_ && ended_at_) {
        try {
            // Parse ISO timestamps and calculate duration
            // This is a simplified implementation
            // In a real implementation, you'd use a proper datetime parsing library
            return std::chrono::milliseconds(1000); // Placeholder
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

// TraceManager implementation
TraceManager::TraceManager(
    std::shared_ptr<TracingProcessor> processor,
    size_t max_finished_traces
) : processor_(processor), max_finished_traces_(max_finished_traces) {}

void TraceManager::cleanup_finished_traces() {
    if (finished_traces_.size() > max_finished_traces_) {
        size_t to_remove = finished_traces_.size() - max_finished_traces_;
        finished_traces_.erase(finished_traces_.begin(), finished_traces_.begin() + to_remove);
    }
}

std::string TraceManager::create_trace(const std::optional<std::string>& trace_id) {
    std::string id = trace_id.value_or(trace_utils::generate_trace_id());
    
    std::lock_guard<std::mutex> lock(traces_mutex_);
    auto trace = std::make_unique<Trace>(id);
    active_traces_[id] = std::move(trace);
    
    return id;
}

Trace* TraceManager::get_active_trace(const std::string& trace_id) {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    auto it = active_traces_.find(trace_id);
    return (it != active_traces_.end()) ? it->second.get() : nullptr;
}

const Trace* TraceManager::get_finished_trace(const std::string& trace_id) const {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    auto it = std::find_if(finished_traces_.begin(), finished_traces_.end(),
        [&trace_id](const auto& trace) {
            return trace->get_trace_id() == trace_id;
        });
    return (it != finished_traces_.end()) ? it->get() : nullptr;
}

void TraceManager::finish_trace(const std::string& trace_id) {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    auto it = active_traces_.find(trace_id);
    if (it != active_traces_.end()) {
        it->second->finish();
        
        // Send to processor if available
        if (processor_) {
            try {
                auto exported = it->second->export_trace();
                if (exported) {
                    processor_->process_trace(*exported);
                }
            } catch (const std::exception& e) {
                logger::error("Failed to process finished trace: " + std::string(e.what()));
            }
        }
        
        // Move to finished traces
        finished_traces_.push_back(std::move(it->second));
        active_traces_.erase(it);
        
        cleanup_finished_traces();
    }
}

std::vector<std::string> TraceManager::get_active_trace_ids() const {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    std::vector<std::string> result;
    for (const auto& [id, _] : active_traces_) {
        result.push_back(id);
    }
    return result;
}

std::vector<std::string> TraceManager::get_finished_trace_ids() const {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    std::vector<std::string> result;
    for (const auto& trace : finished_traces_) {
        result.push_back(trace->get_trace_id());
    }
    return result;
}

TraceManager::ManagerStats TraceManager::get_stats() const {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    ManagerStats stats;
    
    stats.active_traces = active_traces_.size();
    stats.finished_traces = finished_traces_.size();
    
    for (const auto& [_, trace] : active_traces_) {
        auto trace_stats = trace->get_stats();
        stats.total_spans += trace_stats.total_spans;
        
        // Find oldest active trace
        auto duration = trace->get_duration();
        if (duration && (!stats.oldest_active_trace_duration || duration > stats.oldest_active_trace_duration)) {
            stats.oldest_active_trace_duration = duration;
        }
    }
    
    for (const auto& trace : finished_traces_) {
        auto trace_stats = trace->get_stats();
        stats.total_spans += trace_stats.total_spans;
    }
    
    return stats;
}

void TraceManager::clear_all_traces() {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    active_traces_.clear();
    finished_traces_.clear();
}

std::vector<nlohmann::json> TraceManager::export_all_traces() const {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    std::vector<nlohmann::json> result;
    
    for (const auto& [_, trace] : active_traces_) {
        auto exported = trace->export_trace();
        if (exported) {
            result.push_back(*exported);
        }
    }
    
    for (const auto& trace : finished_traces_) {
        auto exported = trace->export_trace();
        if (exported) {
            result.push_back(*exported);
        }
    }
    
    return result;
}

std::vector<nlohmann::json> TraceManager::export_finished_traces() const {
    std::lock_guard<std::mutex> lock(traces_mutex_);
    std::vector<nlohmann::json> result;
    
    for (const auto& trace : finished_traces_) {
        auto exported = trace->export_trace();
        if (exported) {
            result.push_back(*exported);
        }
    }
    
    return result;
}

void TraceManager::set_processor(std::shared_ptr<TracingProcessor> processor) {
    processor_ = processor;
}

Trace* TraceManager::get_current_trace() {
    auto trace_id = ScopedTracingContext::get_current_trace_id();
    return trace_id ? get_active_trace(*trace_id) : nullptr;
}

std::string TraceManager::start_current_trace(const std::optional<std::string>& trace_id) {
    auto id = create_trace(trace_id);
    ScopedTracingContext::set_current_trace_id(id);
    return id;
}

void TraceManager::finish_current_trace() {
    auto trace_id = ScopedTracingContext::get_current_trace_id();
    if (trace_id) {
        finish_trace(*trace_id);
        ScopedTracingContext::reset_current_trace();
    }
}

// GlobalTraceManager implementation
std::unique_ptr<TraceManager> GlobalTraceManager::instance_;
std::mutex GlobalTraceManager::instance_mutex_;

TraceManager& GlobalTraceManager::instance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (!instance_) {
        instance_ = std::make_unique<TraceManager>();
    }
    return *instance_;
}

void GlobalTraceManager::initialize(
    std::shared_ptr<TracingProcessor> processor,
    size_t max_finished_traces
) {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_ = std::make_unique<TraceManager>(processor, max_finished_traces);
}

void GlobalTraceManager::shutdown() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    instance_.reset();
}

bool GlobalTraceManager::is_initialized() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    return instance_ != nullptr;
}

// TraceGuard implementation
TraceGuard::TraceGuard(const std::optional<std::string>& trace_id)
    : should_finish_(true), scope_(ScopedTracingContext::create_disabled_scope()) {
    trace_id_ = GlobalTraceManager::instance().start_current_trace(trace_id);
    scope_ = ScopedTracingContext::create_trace_scope(trace_id_);
}

TraceGuard::TraceGuard(const std::string& existing_trace_id, bool should_finish)
    : trace_id_(existing_trace_id), should_finish_(should_finish),
      scope_(ScopedTracingContext::create_trace_scope(existing_trace_id)) {}

TraceGuard::~TraceGuard() {
    if (should_finish_) {
        GlobalTraceManager::instance().finish_trace(trace_id_);
    }
}

Trace* TraceGuard::get_trace() const {
    return GlobalTraceManager::instance().get_active_trace(trace_id_);
}

// Utility functions
namespace trace_utils {

std::string generate_trace_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "trace_";
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::string generate_span_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "span_";
    for (int i = 0; i < 12; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

std::string get_or_create_current_trace() {
    auto trace_id = ScopedTracingContext::get_current_trace_id();
    if (trace_id) {
        return *trace_id;
    }
    
    return GlobalTraceManager::instance().start_current_trace();
}

bool has_active_trace() {
    return ScopedTracingContext::get_current_trace_id().has_value();
}

} // namespace trace_utils

} // namespace tracing
} // namespace openai_agents