#include "_config.h"

namespace openai_agents {

std::string Config::openai_api_key_;
std::string Config::openai_api_type_ = "responses";
bool Config::use_key_for_tracing_ = true;

void Config::set_default_openai_key(const std::string& key, bool use_for_tracing) {
    openai_api_key_ = key;
    use_key_for_tracing_ = use_for_tracing;
}

void Config::set_default_openai_api(const std::string& api) {
    openai_api_type_ = api;
}

std::string Config::get_openai_api_key() {
    return openai_api_key_;
}

} // namespace openai_agents