#pragma once

/**
 * OpenAI Agents Framework - Tracing Module
 * 
 * This module provides comprehensive distributed tracing capabilities
 * for monitoring and debugging agent operations.
 * 
 * Key Features:
 * - Distributed trace and span management
 * - Multiple span types (Agent, Function, Generation, etc.)
 * - Pluggable processor architecture
 * - Context propagation
 * - Async processing support
 * - Multiple export formats
 * 
 * Usage:
 * ```cpp
 * #include "tracing/tracing.h"
 * 
 * // Initialize tracing
 * openai_agents::tracing::quick_setup::console();
 * 
 * // Create spans
 * auto span = openai_agents::tracing::create::function_span("my_function");
 * auto guard = openai_agents::tracing::make_span_guard(std::move(span));
 * 
 * // ... your code here ...
 * 
 * // Span automatically finishes when guard goes out of scope
 * ```
 */

// Core tracing components
#include "span_data.h"
#include "spans.h"
#include "traces.h"
#include "scope.h"
#include "create.h"

// Processing and export
#include "processor_interface.h"
#include "processors.h"

// Setup and configuration
#include "setup.h"

// Utilities
#include "util.h"

/**
 * Main tracing namespace
 */
namespace openai_agents {
namespace tracing {

/**
 * Initialize tracing with default console output
 */
inline void init() {
    quick_setup::console();
}

/**
 * Initialize tracing with custom configuration
 */
inline void init(const TracingConfig& config) {
    TracingSetup::initialize(config);
}

/**
 * Shutdown tracing system
 */
inline void shutdown() {
    TracingSetup::shutdown();
}

/**
 * Check if tracing is enabled
 */
inline bool is_enabled() {
    return TracingSetup::is_enabled();
}

/**
 * Enable or disable tracing
 */
inline void set_enabled(bool enabled) {
    TracingSetup::set_enabled(enabled);
}

/**
 * Flush all pending trace data
 */
inline void flush() {
    TracingSetup::flush();
}

/**
 * Get tracing system status
 */
inline nlohmann::json get_status() {
    return TracingSetup::get_status();
}

} // namespace tracing
} // namespace openai_agents

/**
 * Convenience macros for common tracing operations
 */

// Create and automatically manage spans
#define TRACE_SPAN(name) \
    auto _trace_span = ::openai_agents::tracing::create::custom_span(name); \
    auto _trace_guard = ::openai_agents::tracing::make_span_guard(std::move(_trace_span))

#define TRACE_FUNCTION() \
    auto _trace_span = ::openai_agents::tracing::create::function_span(__FUNCTION__); \
    auto _trace_guard = ::openai_agents::tracing::make_span_guard(std::move(_trace_span))

#define TRACE_AGENT(agent_name) \
    auto _trace_span = ::openai_agents::tracing::create::agent_span(agent_name); \
    auto _trace_guard = ::openai_agents::tracing::make_span_guard(std::move(_trace_span))

// Set span errors
#define TRACE_ERROR(message) \
    if (_trace_guard.get()) { \
        _trace_guard->set_error(::openai_agents::tracing::SpanError(message)); \
    }

// Conditional tracing
#define TRACE_IF_ENABLED(code) \
    do { \
        if (::openai_agents::tracing::is_enabled()) { \
            code; \
        } \
    } while(0)