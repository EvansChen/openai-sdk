#include "version.h"

namespace openai_agents {

// Fallback version if not built with proper versioning
const std::string VERSION = "0.0.0";

std::string get_version() {
    // In a real build system, this would be populated by CMake or similar
    // For now, return the fallback version
    return VERSION;
}

} // namespace openai_agents