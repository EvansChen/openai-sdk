#pragma once

#include <string>

namespace openai_agents {

/**
 * Get the version string of the openai-agents library
 * @return Version string in the format "x.y.z"
 */
std::string get_version();

/**
 * Version string constant
 */
extern const std::string VERSION;

} // namespace openai_agents