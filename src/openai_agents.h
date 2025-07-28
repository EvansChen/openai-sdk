#pragma once

/**
 * OpenAI Agents C++ SDK
 * 
 * Main header file that includes all public APIs
 */

// Core components
#include "version.h"
#include "exceptions.h"
#include "usage.h"
#include "model_settings.h"

// Agent and related components
#include "agent.h"
#include "agent_output.h"
#include "computer.h"
#include "guardrail.h"
#include "handoffs.h"
#include "items.h"
#include "lifecycle.h"
#include "prompts.h"
#include "result.h"
#include "run.h"
#include "run_context.h"
#include "stream_events.h"
#include "tool.h"

// Memory and storage
#include "memory/session.h"

// Models
#include "models/interface.h"
#include "models/openai_chatcompletions.h"
#include "models/openai_provider.h"
#include "models/openai_responses.h"

// Tracing
#include "tracing/create.h"
#include "tracing/logger.h"
#include "tracing/processor_interface.h"
#include "tracing/processors.h"
#include "tracing/provider.h"
#include "tracing/scope.h"
#include "tracing/setup.h"
#include "tracing/span_data.h"
#include "tracing/spans.h"
#include "tracing/traces.h"
#include "tracing/util.h"

// Utilities
#include "util/_coro.h"
#include "util/_error_tracing.h"
#include "util/_json.h"
#include "util/_pretty_print.h"
#include "util/_transforms.h"
#include "util/_types.h"

// Voice
#include "voice/events.h"
#include "voice/exceptions.h"
#include "voice/imports.h"
#include "voice/input.h"
#include "voice/model.h"
#include "voice/pipeline_config.h"
#include "voice/pipeline.h"
#include "voice/result.h"
#include "voice/utils.h"
#include "voice/workflow.h"

namespace openai_agents {

/**
 * Set the default OpenAI API key to use for LLM requests (and optionally tracing).
 * This is only necessary if the OPENAI_API_KEY environment variable is not already set.
 * 
 * @param key The OpenAI key to use
 * @param use_for_tracing Whether to also use this key to send traces to OpenAI
 */
void set_default_openai_key(const std::string& key, bool use_for_tracing = true);

/**
 * Set the default OpenAI API to use for OpenAI LLM requests.
 * By default, we will use the responses API but you can set this to use the chat completions API instead.
 * 
 * @param api The API type to use ("chat_completions" or "responses")
 */
void set_default_openai_api(const std::string& api);

/**
 * Enables verbose logging to stdout. This is useful for debugging.
 */
void enable_verbose_stdout_logging();

} // namespace openai_agents