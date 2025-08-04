#include "_error_tracing.h"
#include "../tracing/scope.h"
#include <typeinfo>
#include <sstream>

namespace openai_agents {
namespace util {

void attach_error_to_current_span(const tracing::SpanError& error) {
    auto span = tracing::get_current_span();
    if (span) {
        span->set_error(error);
    } else {
        logger::logger().warning("No span to add error '{}' to", error.message);
    }
}

tracing::SpanError create_span_error_from_exception(
    const std::exception& exception,
    const std::optional<std::string>& message,
    const std::unordered_map<std::string, std::any>& data
) {
    std::string error_message = message.value_or(exception.what());
    
    // Create a copy of the data and add exception info
    auto error_data = data;
    error_data["exception_type"] = std::string(typeid(exception).name());
    error_data["exception_what"] = std::string(exception.what());
    
    return tracing::SpanError{
        .message = error_message,
        .data = error_data
    };
}

tracing::SpanError create_span_error_from_exception_ptr(
    std::exception_ptr exception_ptr,
    const std::optional<std::string>& message,
    const std::unordered_map<std::string, std::any>& data
) {
    std::string error_message = message.value_or("Exception occurred");
    auto error_data = data;
    
    try {
        std::rethrow_exception(exception_ptr);
    } catch (const std::exception& e) {
        return create_span_error_from_exception(e, message, data);
    } catch (...) {
        error_message = message.value_or("Unknown exception occurred");
        error_data["exception_type"] = "unknown";
    }
    
    return tracing::SpanError{
        .message = error_message,
        .data = error_data
    };
}

void attach_exception_to_current_span(
    const std::exception& exception,
    const std::optional<std::string>& message,
    const std::unordered_map<std::string, std::any>& data
) {
    auto error = create_span_error_from_exception(exception, message, data);
    attach_error_to_current_span(error);
}

void attach_exception_ptr_to_current_span(
    std::exception_ptr exception_ptr,
    const std::optional<std::string>& message,
    const std::unordered_map<std::string, std::any>& data
) {
    auto error = create_span_error_from_exception_ptr(exception_ptr, message, data);
    attach_error_to_current_span(error);
}

// ErrorTracer implementation
ErrorTracer::ErrorTracer(const std::string& operation_name)
    : operation_name_(operation_name),
      initial_exception_(std::current_exception()),
      error_marked_(false) {
}

ErrorTracer::~ErrorTracer() noexcept {
    if (error_marked_) {
        return; // Already handled
    }
    
    auto current_exception = std::current_exception();
    
    // If there's a new exception that wasn't there when we started
    if (current_exception && current_exception != initial_exception_) {
        try {
            attach_exception_ptr_to_current_span(
                current_exception,
                "Exception in operation: " + operation_name_
            );
        } catch (...) {
            // Can't throw from destructor, just log
            try {
                logger::logger().error("Failed to attach exception to span for operation: {}", operation_name_);
            } catch (...) {
                // Even logging failed, nothing we can do
            }
        }
    }
}

void ErrorTracer::mark_error(const tracing::SpanError& error) {
    attach_error_to_current_span(error);
    error_marked_ = true;
}

void ErrorTracer::mark_exception(
    const std::exception& exception,
    const std::optional<std::string>& message
) {
    auto full_message = message.value_or("Exception in operation: " + operation_name_);
    attach_exception_to_current_span(exception, full_message);
    error_marked_ = true;
}

} // namespace util
} // namespace openai_agents