#pragma once

/**
 * Trace Management for OpenAI Agents Framework
 * 
 * This module provides the core trace management functionality,
 * including trace lifecycle, span collection, and export.
 */

#include "spans.h"
#include "scope.h"
#include "../logger.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <chrono>
#include <atomic>

namespace openai_agents {
namespace tracing {

// Forward declarations
class TracingProcessor;

/**
 * Represents a complete trace with all its spans
 */
class Trace {
private:
    std::string trace_id_;
    std::vector<std::unique_ptr<AnySpan>> spans_;
    std::optional<std::string> started_at_;
    std::optional<std::string> ended_at_;
    std::unordered_map<std::string, std::any> metadata_;
    mutable std::mutex spans_mutex_;
    std::atomic<bool> is_finished_{false};
    
    /**
     * Get current time in ISO format
     */
    std::string current_time_iso() const;
    
public:
    /**
     * Create a new trace with the given ID
     */
    explicit Trace(const std::string& trace_id);
    
    /**
     * Get the trace ID
     */
    const std::string& get_trace_id() const { return trace_id_; }
    
    /**
     * Get the start time
     */
    const std::optional<std::string>& get_started_at() const { return started_at_; }
    
    /**
     * Get the end time
     */
    const std::optional<std::string>& get_ended_at() const { return ended_at_; }
    
    /**
     * Check if the trace is finished
     */
    bool is_finished() const { return is_finished_.load(); }
    
    /**
     * Add a span to this trace
     */
    template<typename TSpanData>
    void add_span(const Span<TSpanData>& span) {
        std::lock_guard<std::mutex> lock(spans_mutex_);
        spans_.push_back(std::make_unique<AnySpan>(span));
        
        // Set start time if this is the first span
        if (spans_.size() == 1 && !started_at_) {
            started_at_ = current_time_iso();
        }
    }
    
    /**
     * Get all spans in this trace
     */
    std::vector<const AnySpan*> get_spans() const;
    
    /**
     * Get spans filtered by type
     */
    template<typename TSpanData>
    std::vector<const AnySpan*> get_spans_by_type() const {
        std::lock_guard<std::mutex> lock(spans_mutex_);
        std::vector<const AnySpan*> result;
        
        for (const auto& span : spans_) {
            // Check if the span data matches the requested type
            try {
                const auto& span_data = span->get_span_data();
                if (dynamic_cast<const TSpanData*>(&span_data)) {
                    result.push_back(span.get());
                }
            } catch (const std::exception&) {
                // Type mismatch, skip
            }
        }
        
        return result;
    }
    
    /**
     * Get a span by its ID
     */
    const AnySpan* get_span_by_id(const std::string& span_id) const;
    
    /**
     * Get the root spans (spans with no parent)
     */
    std::vector<const AnySpan*> get_root_spans() const;
    
    /**
     * Get child spans of a given span
     */
    std::vector<const AnySpan*> get_child_spans(const std::string& parent_span_id) const;
    
    /**
     * Get metadata
     */
    const std::unordered_map<std::string, std::any>& get_metadata() const { return metadata_; }
    
    /**
     * Set metadata
     */
    template<typename T>
    void set_metadata(const std::string& key, const T& value) {
        metadata_[key] = value;
    }
    
    /**
     * Get metadata value
     */
    template<typename T>
    std::optional<T> get_metadata_value(const std::string& key) const {
        auto it = metadata_.find(key);
        if (it != metadata_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
    /**
     * Mark the trace as finished
     */
    void finish();
    
    /**
     * Export the trace as JSON
     */
    std::optional<nlohmann::json> export_trace() const;
    
    /**
     * Get trace statistics
     */
    struct TraceStats {
        size_t total_spans = 0;
        size_t error_spans = 0;
        std::optional<std::chrono::milliseconds> duration;
        std::unordered_map<std::string, size_t> span_types;
    };
    
    TraceStats get_stats() const;
    
    /**
     * Get trace duration in milliseconds
     */
    std::optional<std::chrono::milliseconds> get_duration() const;
    
    // Non-copyable
    Trace(const Trace&) = delete;
    Trace& operator=(const Trace&) = delete;
    
    // Movable
    Trace(Trace&&) = default;
    Trace& operator=(Trace&&) = default;
};

/**
 * Trace manager that handles trace lifecycle and collection
 */
class TraceManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Trace>> active_traces_;
    std::vector<std::unique_ptr<Trace>> finished_traces_;
    mutable std::mutex traces_mutex_;
    std::shared_ptr<TracingProcessor> processor_;
    size_t max_finished_traces_;
    std::atomic<size_t> trace_counter_{0};
    
    /**
     * Clean up old finished traces
     */
    void cleanup_finished_traces();
    
public:
    /**
     * Create a trace manager
     */
    explicit TraceManager(
        std::shared_ptr<TracingProcessor> processor = nullptr,
        size_t max_finished_traces = 1000
    );
    
    /**
     * Create a new trace
     */
    std::string create_trace(const std::optional<std::string>& trace_id = std::nullopt);
    
    /**
     * Get an active trace by ID
     */
    Trace* get_active_trace(const std::string& trace_id);
    
    /**
     * Get a finished trace by ID
     */
    const Trace* get_finished_trace(const std::string& trace_id) const;
    
    /**
     * Add a span to a trace
     */
    template<typename TSpanData>
    void add_span_to_trace(const std::string& trace_id, const Span<TSpanData>& span) {
        std::lock_guard<std::mutex> lock(traces_mutex_);
        auto it = active_traces_.find(trace_id);
        if (it != active_traces_.end()) {
            it->second->add_span(span);
        }
    }
    
    /**
     * Finish a trace
     */
    void finish_trace(const std::string& trace_id);
    
    /**
     * Get all active trace IDs
     */
    std::vector<std::string> get_active_trace_ids() const;
    
    /**
     * Get all finished trace IDs
     */
    std::vector<std::string> get_finished_trace_ids() const;
    
    /**
     * Get trace statistics
     */
    struct ManagerStats {
        size_t active_traces = 0;
        size_t finished_traces = 0;
        size_t total_spans = 0;
        std::optional<std::chrono::milliseconds> oldest_active_trace_duration;
    };
    
    ManagerStats get_stats() const;
    
    /**
     * Clear all traces
     */
    void clear_all_traces();
    
    /**
     * Export all traces as JSON
     */
    std::vector<nlohmann::json> export_all_traces() const;
    
    /**
     * Export finished traces as JSON
     */
    std::vector<nlohmann::json> export_finished_traces() const;
    
    /**
     * Set the tracing processor
     */
    void set_processor(std::shared_ptr<TracingProcessor> processor);
    
    /**
     * Get the current trace from context
     */
    Trace* get_current_trace();
    
    /**
     * Create a trace and set it as current
     */
    std::string start_current_trace(const std::optional<std::string>& trace_id = std::nullopt);
    
    /**
     * Finish the current trace
     */
    void finish_current_trace();
    
    // Non-copyable
    TraceManager(const TraceManager&) = delete;
    TraceManager& operator=(const TraceManager&) = delete;
    
    // Movable
    TraceManager(TraceManager&&) = default;
    TraceManager& operator=(TraceManager&&) = default;
};

/**
 * Global trace manager instance
 */
class GlobalTraceManager {
private:
    static std::unique_ptr<TraceManager> instance_;
    static std::mutex instance_mutex_;
    
public:
    /**
     * Get the global trace manager instance
     */
    static TraceManager& instance();
    
    /**
     * Initialize the global trace manager
     */
    static void initialize(
        std::shared_ptr<TracingProcessor> processor = nullptr,
        size_t max_finished_traces = 1000
    );
    
    /**
     * Shutdown the global trace manager
     */
    static void shutdown();
    
    /**
     * Check if the global trace manager is initialized
     */
    static bool is_initialized();
};

/**
 * RAII trace guard for automatic trace management
 */
class TraceGuard {
private:
    std::string trace_id_;
    bool should_finish_;
    ScopedContext scope_;
    
public:
    /**
     * Create a trace guard with a new trace
     */
    explicit TraceGuard(const std::optional<std::string>& trace_id = std::nullopt);
    
    /**
     * Create a trace guard with an existing trace
     */
    explicit TraceGuard(const std::string& existing_trace_id, bool should_finish);
    
    /**
     * Destructor - finish trace if needed
     */
    ~TraceGuard();
    
    /**
     * Get the trace ID
     */
    const std::string& get_trace_id() const { return trace_id_; }
    
    /**
     * Get the trace
     */
    Trace* get_trace() const;
    
    /**
     * Disable automatic finishing
     */
    void disable_auto_finish() { should_finish_ = false; }
    
    // Non-copyable, movable
    TraceGuard(const TraceGuard&) = delete;
    TraceGuard& operator=(const TraceGuard&) = delete;
    
    TraceGuard(TraceGuard&& other) noexcept
        : trace_id_(std::move(other.trace_id_)),
          should_finish_(other.should_finish_),
          scope_(std::move(other.scope_)) {
        other.should_finish_ = false;
    }
    
    TraceGuard& operator=(TraceGuard&& other) noexcept {
        if (this != &other) {
            if (should_finish_) {
                GlobalTraceManager::instance().finish_trace(trace_id_);
            }
            trace_id_ = std::move(other.trace_id_);
            should_finish_ = other.should_finish_;
            scope_ = std::move(other.scope_);
            other.should_finish_ = false;
        }
        return *this;
    }
};

/**
 * Utility functions for trace management
 */
namespace trace_utils {

/**
 * Generate a unique trace ID
 */
std::string generate_trace_id();

/**
 * Generate a unique span ID
 */
std::string generate_span_id();

/**
 * Run a function within a trace context
 */
template<typename Func>
auto with_trace(Func&& func, const std::optional<std::string>& trace_id = std::nullopt) -> decltype(func()) {
    auto guard = TraceGuard(trace_id);
    return func();
}

/**
 * Get the current trace or create a new one
 */
std::string get_or_create_current_trace();

/**
 * Check if there's an active trace
 */
bool has_active_trace();

} // namespace trace_utils

} // namespace tracing
} // namespace openai_agents