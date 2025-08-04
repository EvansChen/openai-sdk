#include "scope.h"

namespace openai_agents {
namespace tracing {

// Thread-local storage for tracing context
thread_local TracingContext ThreadLocalContext::context_;

} // namespace tracing
} // namespace openai_agents