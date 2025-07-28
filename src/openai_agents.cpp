#include "openai_agents.h"
#include "_config.h"
#include <iostream>

namespace openai_agents {

void set_default_openai_key(const std::string& key, bool use_for_tracing) {
    // Implementation would call into config module
    // For now, just placeholder
    // config::set_default_openai_key(key, use_for_tracing);
}

void set_default_openai_api(const std::string& api) {
    // Implementation would call into config module
    // For now, just placeholder
    // config::set_default_openai_api(api);
}

void enable_verbose_stdout_logging() {
    // Implementation would configure logging
    // For now, just enable console output
    std::cout << "Verbose logging enabled" << std::endl;
}

} // namespace openai_agents