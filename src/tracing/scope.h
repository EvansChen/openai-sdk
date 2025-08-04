#pragma once

/**
 * Tracing Context and Scope Management for OpenAI Agents Framework
 * 
 * This module provides context management for distributed tracing,
 * similar to Python's contextvars but implemented for C++.
 */

#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include <any>

namespace openai_agents {
namespace tracing {

/**
 * Thread-local context storage for tracing information
 * 
 * This class manages tracing context in a thread-safe manner,
 * providing the equivalent of Python's contextvars functionality.
 */
class TracingContext {
private:
    std::unordered_map<std::string, std::any> context_vars_;
    
public:
    /**
     * Get a context variable
     */
    template<typename T>
    std::optional<T> get(const std::string& key) const {
        auto it = context_vars_.find(key);
        if (it != context_vars_.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
    /**
     * Set a context variable
     */
    template<typename T>
    void set(const std::string& key, const T& value) {
        context_vars_[key] = value;
    }
    
    /**
     * Remove a context variable
     */
    void remove(const std::string& key) {
        context_vars_.erase(key);
    }
    
    /**
     * Clear all context variables
     */
    void clear() {
        context_vars_.clear();
    }
    
    /**
     * Check if a context variable exists
     */
    bool has(const std::string& key) const {
        return context_vars_.find(key) != context_vars_.end();
    }
    
    /**
     * Get all context variable keys
     */
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        for (const auto& [key, _] : context_vars_) {
            result.push_back(key);
        }
        return result;
    }
    
    /**
     * Copy constructor
     */
    TracingContext(const TracingContext& other) : context_vars_(other.context_vars_) {}
    
    /**
     * Assignment operator
     */
    TracingContext& operator=(const TracingContext& other) {
        if (this != &other) {
            context_vars_ = other.context_vars_;
        }
        return *this;
    }
    
    /**
     * Default constructor
     */
    TracingContext() = default;
};

/**
 * Thread-local context manager
 * 
 * Provides static access to thread-local tracing context.
 */
class ThreadLocalContext {
private:
    static thread_local TracingContext context_;
    
public:
    /**
     * Get the current thread's tracing context
     */
    static TracingContext& current() {
        return context_;
    }
    
    /**
     * Get a copy of the current context
     */
    static TracingContext copy() {
        return context_;
    }
    
    /**
     * Set the current context
     */
    static void set_context(const TracingContext& context) {
        context_ = context;
    }
    
    /**
     * Clear the current context
     */
    static void clear() {
        context_.clear();
    }
};

/**
 * RAII context manager for scoped context changes
 * 
 * Automatically restores the previous context when the scope ends.
 */
class ScopedContext {
private:
    TracingContext previous_context_;
    bool should_restore_;
    
public:
    /**
     * Create a scoped context with the given context
     */
    explicit ScopedContext(const TracingContext& new_context)
        : previous_context_(ThreadLocalContext::copy()), should_restore_(true) {
        ThreadLocalContext::set_context(new_context);
    }
    
    /**
     * Create a scoped context that modifies the current context
     */
    template<typename T>
    ScopedContext(const std::string& key, const T& value)
        : previous_context_(ThreadLocalContext::copy()), should_restore_(true) {
        ThreadLocalContext::current().set(key, value);
    }
    
    /**
     * Destructor - restore previous context
     */
    ~ScopedContext() {
        if (should_restore_) {
            ThreadLocalContext::set_context(previous_context_);
        }
    }
    
    /**
     * Disable restoration (for early exit scenarios)
     */
    void disable_restore() {
        should_restore_ = false;
    }
    
    /**
     * Get the previous context
     */
    const TracingContext& get_previous_context() const {
        return previous_context_;
    }
    
    // Non-copyable, movable
    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;
    
    ScopedContext(ScopedContext&& other) noexcept
        : previous_context_(std::move(other.previous_context_)),
          should_restore_(other.should_restore_) {
        other.should_restore_ = false;
    }
    
    ScopedContext& operator=(ScopedContext&& other) noexcept {
        if (this != &other) {
            if (should_restore_) {
                ThreadLocalContext::set_context(previous_context_);
            }
            previous_context_ = std::move(other.previous_context_);
            should_restore_ = other.should_restore_;
            other.should_restore_ = false;
        }
        return *this;
    }
};

/**
 * Specialized tracing context management
 * 
 * Provides high-level APIs for common tracing context operations.
 */
class ScopedTracingContext {
public:
    // Context variable keys
    static constexpr const char* CURRENT_TRACE_ID = "current_trace_id";
    static constexpr const char* CURRENT_SPAN_ID = "current_span_id";
    static constexpr const char* TRACE_DISABLED = "trace_disabled";
    
    /**
     * Get the current trace ID
     */
    static std::optional<std::string> get_current_trace_id() {
        return ThreadLocalContext::current().get<std::string>(CURRENT_TRACE_ID);
    }
    
    /**
     * Set the current trace ID
     */
    static void set_current_trace_id(const std::string& trace_id) {
        ThreadLocalContext::current().set(CURRENT_TRACE_ID, trace_id);
    }
    
    /**
     * Reset the current trace ID
     */
    static void reset_current_trace() {
        ThreadLocalContext::current().remove(CURRENT_TRACE_ID);
    }
    
    /**
     * Get the current span ID
     */
    static std::optional<std::string> get_current_span_id() {
        return ThreadLocalContext::current().get<std::string>(CURRENT_SPAN_ID);
    }
    
    /**
     * Set the current span ID
     */
    static void set_current_span_id(const std::string& span_id) {
        ThreadLocalContext::current().set(CURRENT_SPAN_ID, span_id);
    }
    
    /**
     * Reset the current span ID
     */
    static void reset_current_span() {
        ThreadLocalContext::current().remove(CURRENT_SPAN_ID);
    }
    
    /**
     * Check if tracing is disabled
     */
    static bool is_trace_disabled() {
        auto disabled = ThreadLocalContext::current().get<bool>(TRACE_DISABLED);
        return disabled.value_or(false);
    }
    
    /**
     * Disable tracing for the current context
     */
    static void disable_tracing() {
        ThreadLocalContext::current().set(TRACE_DISABLED, true);
    }
    
    /**
     * Enable tracing for the current context
     */
    static void enable_tracing() {
        ThreadLocalContext::current().remove(TRACE_DISABLED);
    }
    
    /**
     * Create a scoped trace context
     */
    static ScopedContext create_trace_scope(const std::string& trace_id) {
        return ScopedContext(CURRENT_TRACE_ID, trace_id);
    }
    
    /**
     * Create a scoped span context
     */
    static ScopedContext create_span_scope(const std::string& span_id) {
        return ScopedContext(CURRENT_SPAN_ID, span_id);
    }
    
    /**
     * Create a scoped trace and span context
     */
    static ScopedContext create_trace_span_scope(const std::string& trace_id, const std::string& span_id) {
        auto context = ThreadLocalContext::copy();
        context.set(CURRENT_TRACE_ID, trace_id);
        context.set(CURRENT_SPAN_ID, span_id);
        return ScopedContext(context);
    }
    
    /**
     * Create a scoped disabled tracing context
     */
    static ScopedContext create_disabled_scope() {
        return ScopedContext(TRACE_DISABLED, true);
    }
    
    /**
     * Clear all tracing context
     */
    static void clear_all() {
        ThreadLocalContext::clear();
    }
    
    /**
     * Get a snapshot of the current tracing context
     */
    static std::unordered_map<std::string, std::string> get_context_snapshot() {
        std::unordered_map<std::string, std::string> snapshot;
        
        auto trace_id = get_current_trace_id();
        if (trace_id) {
            snapshot[CURRENT_TRACE_ID] = *trace_id;
        }
        
        auto span_id = get_current_span_id();
        if (span_id) {
            snapshot[CURRENT_SPAN_ID] = *span_id;
        }
        
        if (is_trace_disabled()) {
            snapshot[TRACE_DISABLED] = "true";
        }
        
        return snapshot;
    }
    
    /**
     * Restore context from a snapshot
     */
    static ScopedContext restore_from_snapshot(const std::unordered_map<std::string, std::string>& snapshot) {
        auto context = TracingContext();
        
        auto trace_it = snapshot.find(CURRENT_TRACE_ID);
        if (trace_it != snapshot.end()) {
            context.set(CURRENT_TRACE_ID, trace_it->second);
        }
        
        auto span_it = snapshot.find(CURRENT_SPAN_ID);
        if (span_it != snapshot.end()) {
            context.set(CURRENT_SPAN_ID, span_it->second);
        }
        
        auto disabled_it = snapshot.find(TRACE_DISABLED);
        if (disabled_it != snapshot.end() && disabled_it->second == "true") {
            context.set(TRACE_DISABLED, true);
        }
        
        return ScopedContext(context);
    }
};

/**
 * Utility functions for context management
 */
namespace context_utils {

/**
 * Run a function with a specific tracing context
 */
template<typename Func>
auto with_trace_context(const std::string& trace_id, Func&& func) -> decltype(func()) {
    auto scope = ScopedTracingContext::create_trace_scope(trace_id);
    return func();
}

/**
 * Run a function with a specific span context
 */
template<typename Func>
auto with_span_context(const std::string& span_id, Func&& func) -> decltype(func()) {
    auto scope = ScopedTracingContext::create_span_scope(span_id);
    return func();
}

/**
 * Run a function with tracing disabled
 */
template<typename Func>
auto with_disabled_tracing(Func&& func) -> decltype(func()) {
    auto scope = ScopedTracingContext::create_disabled_scope();
    return func();
}

/**
 * Run a function with a complete trace and span context
 */
template<typename Func>
auto with_trace_span_context(const std::string& trace_id, const std::string& span_id, Func&& func) -> decltype(func()) {
    auto scope = ScopedTracingContext::create_trace_span_scope(trace_id, span_id);
    return func();
}

} // namespace context_utils

} // namespace tracing
} // namespace openai_agents