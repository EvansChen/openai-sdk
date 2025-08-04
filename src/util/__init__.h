#pragma once

/**
 * Utility Module for OpenAI Agents Framework
 * 
 * This module provides various utility functions and types used throughout
 * the OpenAI Agents framework. It includes:
 * 
 * - Type utilities for async programming and type manipulation
 * - Coroutine utilities for async operations (using std::future)
 * - Error tracing utilities for better debugging and monitoring
 * - JSON validation and parsing utilities
 * - String transformation and formatting utilities
 * - Pretty printing utilities for displaying framework objects
 * 
 * @example Basic Usage
 * ```cpp
 * #include "util/__init__.h"
 * 
 * using namespace openai_agents::util;
 * 
 * // Type utilities
 * MaybeAwaitable<int> result = make_immediate(42);
 * int value = resolve(std::move(result));
 * 
 * // String transformations
 * std::string func_name = transform_string_function_style("My Function Name");
 * std::string camel_case = transform_string_camel_case("my function name");
 * 
 * // JSON validation
 * MyStruct obj = validate_json<MyStruct>("{\"field\": \"value\"}");
 * 
 * // Error tracing
 * try {
 *     TRACE_ERRORS("my_operation");
 *     // ... code that might throw ...
 * } catch (...) {
 *     // Error automatically attached to current span
 * }
 * 
 * // Pretty printing
 * std::string formatted = pretty_print_result(run_result);
 * ```
 */

// Type utilities
#include "_types.h"

// Coroutine utilities  
#include "_coro.h"

// Error tracing utilities
#include "_error_tracing.h"

// JSON utilities
#include "_json.h"

// String transformation utilities
#include "_transforms.h"

// Pretty printing utilities
#include "_pretty_print.h"

namespace openai_agents {

/**
 * Utility namespace containing helper functions and types
 */
namespace util {

// Re-export commonly used types for convenience
using String = std::string;
using StringView = std::string_view;

/**
 * Utility class for common operations
 */
class Utils {
public:
    /**
     * Create a future that resolves immediately
     */
    template<typename T>
    static std::future<T> immediate(T value) {
        return immediate_future(std::move(value));
    }
    
    /**
     * Create a void future that resolves immediately
     */
    static std::future<void> immediate_void() {
        return immediate_void_future();
    }
    
    /**
     * Transform string to function style
     */
    static std::string to_function_style(const std::string& name) {
        return transform_string_function_style(name);
    }
    
    /**
     * Transform string to camelCase
     */
    static std::string to_camel_case(const std::string& name) {
        return transform_string_camel_case(name);
    }
    
    /**
     * Validate JSON string
     */
    template<typename T>
    static T from_json(const std::string& json_str, bool partial = false) {
        return validate_json<T>(json_str, partial);
    }
    
    /**
     * Check if JSON is valid
     */
    static bool is_json_valid(const std::string& json_str) {
        return is_valid_json(json_str);
    }
    
    /**
     * Trim whitespace from string
     */
    static std::string trim_string(const std::string& text) {
        return trim(text);
    }
    
    /**
     * Format duration in human-readable format
     */
    static std::string format_time(long long milliseconds) {
        return format_duration(milliseconds);
    }
    
    /**
     * Format file size in human-readable format
     */
    static std::string format_size(size_t bytes) {
        return format_file_size(bytes);
    }
};

/**
 * RAII helper for timing operations
 */
class Timer {
public:
    Timer() : start_time_(std::chrono::high_resolution_clock::now()) {}
    
    /**
     * Get elapsed time in milliseconds
     */
    long long elapsed_ms() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time_
        );
        return duration.count();
    }
    
    /**
     * Get elapsed time in seconds
     */
    double elapsed_seconds() const {
        return elapsed_ms() / 1000.0;
    }
    
    /**
     * Reset the timer
     */
    void reset() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    /**
     * Get formatted elapsed time
     */
    std::string elapsed_formatted() const {
        return format_duration(elapsed_ms());
    }

private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * Scope guard for automatic cleanup
 */
template<typename F>
class ScopeGuard {
public:
    explicit ScopeGuard(F&& func) : func_(std::forward<F>(func)), active_(true) {}
    
    ~ScopeGuard() {
        if (active_) {
            try {
                func_();
            } catch (...) {
                // Ignore exceptions in destructor
            }
        }
    }
    
    // Non-copyable
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    
    // Movable
    ScopeGuard(ScopeGuard&& other) noexcept 
        : func_(std::move(other.func_)), active_(other.active_) {
        other.active_ = false;
    }
    
    ScopeGuard& operator=(ScopeGuard&& other) noexcept {
        if (this != &other) {
            if (active_) {
                func_();
            }
            func_ = std::move(other.func_);
            active_ = other.active_;
            other.active_ = false;
        }
        return *this;
    }
    
    /**
     * Dismiss the guard (don't execute the function)
     */
    void dismiss() {
        active_ = false;
    }

private:
    F func_;
    bool active_;
};

/**
 * Create a scope guard
 */
template<typename F>
ScopeGuard<F> make_scope_guard(F&& func) {
    return ScopeGuard<F>(std::forward<F>(func));
}

/**
 * Macro for creating scope guards
 */
#define SCOPE_GUARD(code) \
    auto CONCAT(_scope_guard_, __LINE__) = openai_agents::util::make_scope_guard([&]() { code; })

/**
 * Helper macro for concatenating tokens
 */
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_IMPL(a, b) a##b

/**
 * Defer execution of code to end of scope
 */
#define DEFER(code) SCOPE_GUARD(code)

} // namespace util
} // namespace openai_agents