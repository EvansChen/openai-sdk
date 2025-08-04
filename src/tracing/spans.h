#pragma once

/**
 * Span Types for OpenAI Agents Framework Tracing
 * 
 * This module defines span types that represent individual operations
 * within a trace.
 */

#include "span_data.h"
#include "../logger.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <any>
#include <optional>
#include <chrono>

namespace openai_agents {
namespace tracing {

// Forward declarations
class TracingProcessor;

/**
 * Represents an error that occurred during span execution
 */
struct SpanError {
    std::string message;
    std::optional<std::unordered_map<std::string, std::any>> data;
    
    SpanError(const std::string& message, 
              const std::optional<std::unordered_map<std::string, std::any>>& data = std::nullopt)
        : message(message), data(data) {}
};

/**
 * Base span interface
 */
template<typename TSpanData>
class Span {
public:
    virtual ~Span() = default;
    
    /**
     * Get the trace ID this span belongs to
     */
    virtual const std::string& get_trace_id() const = 0;
    
    /**
     * Get the unique span ID
     */
    virtual const std::string& get_span_id() const = 0;
    
    /**
     * Get the span data
     */
    virtual const TSpanData& get_span_data() const = 0;
    
    /**
     * Get the parent span ID (if any)
     */
    virtual const std::optional<std::string>& get_parent_id() const = 0;
    
    /**
     * Start the span
     * 
     * @param mark_as_current Whether to mark this span as the current active span
     */
    virtual void start(bool mark_as_current = false) = 0;
    
    /**
     * Finish the span
     * 
     * @param reset_current Whether to reset the current span context
     */
    virtual void finish(bool reset_current = false) = 0;
    
    /**
     * Set an error on this span
     */
    virtual void set_error(const SpanError& error) = 0;
    
    /**
     * Get the error (if any) on this span
     */
    virtual const std::optional<SpanError>& get_error() const = 0;
    
    /**
     * Get the start time
     */
    virtual const std::optional<std::string>& get_started_at() const = 0;
    
    /**
     * Get the end time
     */
    virtual const std::optional<std::string>& get_ended_at() const = 0;
    
    /**
     * Export the span as JSON
     */
    virtual std::optional<nlohmann::json> export_span() const = 0;
    
    /**
     * RAII support - start span when entering scope
     */
    virtual Span<TSpanData>& enter() = 0;
    
    /**
     * RAII support - finish span when exiting scope
     */
    virtual void exit() = 0;
};

/**
 * No-op span implementation that doesn't record anything
 */
template<typename TSpanData>
class NoOpSpan : public Span<TSpanData> {
private:
    TSpanData span_data_;
    bool is_current_;
    
public:
    explicit NoOpSpan(const TSpanData& span_data) 
        : span_data_(span_data), is_current_(false) {}
    
    const std::string& get_trace_id() const override {
        static const std::string no_op = "no-op";
        return no_op;
    }
    
    const std::string& get_span_id() const override {
        static const std::string no_op = "no-op";
        return no_op;
    }
    
    const TSpanData& get_span_data() const override {
        return span_data_;
    }
    
    const std::optional<std::string>& get_parent_id() const override {
        static const std::optional<std::string> no_parent = std::nullopt;
        return no_parent;
    }
    
    void start(bool mark_as_current = false) override {
        is_current_ = mark_as_current;
    }
    
    void finish(bool reset_current = false) override {
        if (reset_current) {
            is_current_ = false;
        }
    }
    
    void set_error(const SpanError& error) override {
        // No-op
    }
    
    const std::optional<SpanError>& get_error() const override {
        static const std::optional<SpanError> no_error = std::nullopt;
        return no_error;
    }
    
    const std::optional<std::string>& get_started_at() const override {
        static const std::optional<std::string> no_time = std::nullopt;
        return no_time;
    }
    
    const std::optional<std::string>& get_ended_at() const override {
        static const std::optional<std::string> no_time = std::nullopt;
        return no_time;
    }
    
    std::optional<nlohmann::json> export_span() const override {
        return std::nullopt;
    }
    
    Span<TSpanData>& enter() override {
        start(true);
        return *this;
    }
    
    void exit() override {
        finish(true);
    }
};

/**
 * Real span implementation that records trace data
 */
template<typename TSpanData>
class SpanImpl : public Span<TSpanData> {
private:
    std::string trace_id_;
    std::string span_id_;
    std::optional<std::string> parent_id_;
    TSpanData span_data_;
    std::optional<std::string> started_at_;
    std::optional<std::string> ended_at_;
    std::optional<SpanError> error_;
    std::shared_ptr<TracingProcessor> processor_;
    bool is_current_;
    
    /**
     * Get current time in ISO format
     */
    std::string current_time_iso() const;
    
public:
    SpanImpl(
        const std::string& trace_id,
        const std::string& span_id,
        const std::optional<std::string>& parent_id,
        const TSpanData& span_data,
        std::shared_ptr<TracingProcessor> processor
    );
    
    const std::string& get_trace_id() const override {
        return trace_id_;
    }
    
    const std::string& get_span_id() const override {
        return span_id_;
    }
    
    const TSpanData& get_span_data() const override {
        return span_data_;
    }
    
    const std::optional<std::string>& get_parent_id() const override {
        return parent_id_;
    }
    
    void start(bool mark_as_current = false) override;
    
    void finish(bool reset_current = false) override;
    
    void set_error(const SpanError& error) override {
        error_ = error;
    }
    
    const std::optional<SpanError>& get_error() const override {
        return error_;
    }
    
    const std::optional<std::string>& get_started_at() const override {
        return started_at_;
    }
    
    const std::optional<std::string>& get_ended_at() const override {
        return ended_at_;
    }
    
    std::optional<nlohmann::json> export_span() const override;
    
    Span<TSpanData>& enter() override {
        start(true);
        return *this;
    }
    
    void exit() override {
        finish(true);
    }
};

/**
 * RAII wrapper for automatic span management
 */
template<typename TSpanData>
class SpanGuard {
private:
    std::unique_ptr<Span<TSpanData>> span_;
    
public:
    explicit SpanGuard(std::unique_ptr<Span<TSpanData>> span) 
        : span_(std::move(span)) {
        if (span_) {
            span_->enter();
        }
    }
    
    ~SpanGuard() {
        if (span_) {
            span_->exit();
        }
    }
    
    // Non-copyable
    SpanGuard(const SpanGuard&) = delete;
    SpanGuard& operator=(const SpanGuard&) = delete;
    
    // Movable
    SpanGuard(SpanGuard&& other) noexcept : span_(std::move(other.span_)) {}
    SpanGuard& operator=(SpanGuard&& other) noexcept {
        if (this != &other) {
            if (span_) {
                span_->exit();
            }
            span_ = std::move(other.span_);
        }
        return *this;
    }
    
    /**
     * Get the underlying span
     */
    Span<TSpanData>* get() const {
        return span_.get();
    }
    
    /**
     * Get the underlying span
     */
    Span<TSpanData>& operator*() const {
        return *span_;
    }
    
    /**
     * Get the underlying span
     */
    Span<TSpanData>* operator->() const {
        return span_.get();
    }
    
    /**
     * Release the span without calling exit
     */
    std::unique_ptr<Span<TSpanData>> release() {
        return std::move(span_);
    }
};

/**
 * Helper function to create a span guard
 */
template<typename TSpanData>
SpanGuard<TSpanData> make_span_guard(std::unique_ptr<Span<TSpanData>> span) {
    return SpanGuard<TSpanData>(std::move(span));
}

/**
 * Type aliases for commonly used span types
 */
using AgentSpan = Span<AgentSpanData>;
using FunctionSpan = Span<FunctionSpanData>;
using GenerationSpan = Span<GenerationSpanData>;
using ResponseSpan = Span<ResponseSpanData>;
using HandoffSpan = Span<HandoffSpanData>;
using CustomSpan = Span<CustomSpanData>;
using GuardrailSpan = Span<GuardrailSpanData>;
using TranscriptionSpan = Span<TranscriptionSpanData>;
using SpeechSpan = Span<SpeechSpanData>;
using SpeechGroupSpan = Span<SpeechGroupSpanData>;
using MCPListToolsSpan = Span<MCPListToolsSpanData>;

/**
 * Type-erased span interface for heterogeneous collections
 */
class AnySpan {
private:
    std::string trace_id_;
    std::string span_id_;
    std::optional<std::string> parent_id_;
    std::unique_ptr<SpanData> span_data_;
    std::optional<std::string> started_at_;
    std::optional<std::string> ended_at_;
    std::optional<SpanError> error_;
    
public:
    template<typename TSpanData>
    AnySpan(const Span<TSpanData>& span)
        : trace_id_(span.get_trace_id()),
          span_id_(span.get_span_id()),
          parent_id_(span.get_parent_id()),
          span_data_(span.get_span_data().clone()),
          started_at_(span.get_started_at()),
          ended_at_(span.get_ended_at()),
          error_(span.get_error()) {}
    
    const std::string& get_trace_id() const { return trace_id_; }
    const std::string& get_span_id() const { return span_id_; }
    const std::optional<std::string>& get_parent_id() const { return parent_id_; }
    const SpanData& get_span_data() const { return *span_data_; }
    const std::optional<std::string>& get_started_at() const { return started_at_; }
    const std::optional<std::string>& get_ended_at() const { return ended_at_; }
    const std::optional<SpanError>& get_error() const { return error_; }
    
    std::optional<nlohmann::json> export_span() const;
};

} // namespace tracing
} // namespace openai_agents