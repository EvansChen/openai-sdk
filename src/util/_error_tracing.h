#pragma once

/**
 * Error Tracing Utilities for OpenAI Agents Framework
 * 
 * This module provides utilities for attaching errors to tracing spans,
 * enabling better error tracking and debugging across the framework.
 */

#include "../tracing/spans.h"
#include "../tracing/scope.h"
#include "../logger.h"
#include <memory>
#include <any>

namespace openai_agents {
namespace util {

/**
 * Attach an error to a specific span
 * 
 * This function adds error information to a tracing span, allowing
 * for better error tracking and debugging.
 * 
 * @tparam T The span data type
 * @param span The span to attach the error to
 * @param error The error to attach
 */
template<typename T>
void attach_error_to_span(std::shared_ptr<tracing::Span<T>> span, const tracing::SpanError& error) {
    if (span) {
        span->set_error(error);
    }
}

/**
 * Attach an error to the current active span
 * 
 * This function attempts to get the current span from the tracing
 * context and attach the error to it. If no span is active, it
 * logs a warning.
 * 
 * @param error The error to attach to the current span
 */
void attach_error_to_current_span(const tracing::SpanError& error);

/**
 * Create a SpanError from an exception
 * 
 * @param exception The exception to convert
 * @param message Optional custom message (uses exception.what() if not provided)
 * @param data Optional additional data
 * @return SpanError object
 */
tracing::SpanError create_span_error_from_exception(
    const std::exception& exception,
    const std::optional<std::string>& message = std::nullopt,
    const std::unordered_map<std::string, std::any>& data = {}
);

/**
 * Create a SpanError from an exception pointer
 * 
 * @param exception_ptr The exception pointer to convert
 * @param message Optional custom message
 * @param data Optional additional data
 * @return SpanError object
 */
tracing::SpanError create_span_error_from_exception_ptr(
    std::exception_ptr exception_ptr,
    const std::optional<std::string>& message = std::nullopt,
    const std::unordered_map<std::string, std::any>& data = {}
);

/**
 * Attach an exception to a specific span
 * 
 * @tparam T The span data type
 * @param span The span to attach the error to
 * @param exception The exception to attach
 * @param message Optional custom message
 * @param data Optional additional data
 */
template<typename T>
void attach_exception_to_span(
    std::shared_ptr<tracing::Span<T>> span,
    const std::exception& exception,
    const std::optional<std::string>& message = std::nullopt,
    const std::unordered_map<std::string, std::any>& data = {}
) {
    auto error = create_span_error_from_exception(exception, message, data);
    attach_error_to_span(span, error);
}

/**
 * Attach an exception to the current active span
 * 
 * @param exception The exception to attach
 * @param message Optional custom message
 * @param data Optional additional data
 */
void attach_exception_to_current_span(
    const std::exception& exception,
    const std::optional<std::string>& message = std::nullopt,
    const std::unordered_map<std::string, std::any>& data = {}
);

/**
 * Attach an exception pointer to the current active span
 * 
 * @param exception_ptr The exception pointer to attach
 * @param message Optional custom message
 * @param data Optional additional data
 */
void attach_exception_ptr_to_current_span(
    std::exception_ptr exception_ptr,
    const std::optional<std::string>& message = std::nullopt,
    const std::unordered_map<std::string, std::any>& data = {}
);

/**
 * RAII helper for automatic error tracing
 * 
 * This class automatically attaches any uncaught exceptions to the current
 * span when it goes out of scope.
 */
class ErrorTracer {
public:
    /**
     * Constructor
     * 
     * @param operation_name Name of the operation being traced
     */
    explicit ErrorTracer(const std::string& operation_name);

    /**
     * Destructor - automatically handles any uncaught exceptions
     */
    ~ErrorTracer() noexcept;

    /**
     * Manually mark an error
     * 
     * @param error The error to attach
     */
    void mark_error(const tracing::SpanError& error);

    /**
     * Manually mark an exception
     * 
     * @param exception The exception to attach
     * @param message Optional custom message
     */
    void mark_exception(
        const std::exception& exception,
        const std::optional<std::string>& message = std::nullopt
    );

private:
    std::string operation_name_;
    std::exception_ptr initial_exception_;
    bool error_marked_;
};

/**
 * Macro for convenient error tracing
 * 
 * Usage:
 * {
 *     TRACE_ERRORS("my_operation");
 *     // ... code that might throw ...
 * } // Automatically traces any exceptions
 */
#define TRACE_ERRORS(operation_name) \
    openai_agents::util::ErrorTracer _error_tracer(operation_name)

/**
 * Function wrapper that automatically traces errors
 * 
 * @tparam F The function type
 * @tparam Args The argument types
 * @param operation_name Name of the operation
 * @param func The function to wrap
 * @param args The arguments to pass to the function
 * @return The result of the function call
 */
template<typename F, typename... Args>
auto trace_errors(const std::string& operation_name, F&& func, Args&&... args) 
    -> std::invoke_result_t<F, Args...> {
    ErrorTracer tracer(operation_name);
    try {
        return func(std::forward<Args>(args)...);
    } catch (...) {
        // ErrorTracer destructor will handle the exception
        throw;
    }
}

/**
 * Async function wrapper that automatically traces errors
 * 
 * @tparam F The function type
 * @tparam Args The argument types
 * @param operation_name Name of the operation
 * @param func The function to wrap
 * @param args The arguments to pass to the function
 * @return A future representing the async operation with error tracing
 */
template<typename F, typename... Args>
auto trace_errors_async(const std::string& operation_name, F&& func, Args&&... args) 
    -> std::future<std::invoke_result_t<F, Args...>> {
    return std::async(std::launch::async, [operation_name, func = std::forward<F>(func), args...]() mutable {
        return trace_errors(operation_name, func, args...);
    });
}

} // namespace util
} // namespace openai_agents