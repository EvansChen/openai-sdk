#pragma once

namespace openai_agents {

/**
 * Configuration settings for the OpenAI SDK
 */
class Config {
public:
    /**
     * Set the default OpenAI API key
     */
    static void set_default_openai_key(const std::string& key, bool use_for_tracing = true);
    
    /**
     * Set the default OpenAI API type
     */
    static void set_default_openai_api(const std::string& api);
    
    /**
     * Get the current OpenAI API key
     */
    static std::string get_openai_api_key();

private:
    static std::string openai_api_key_;
    static std::string openai_api_type_;
    static bool use_key_for_tracing_;
};

} // namespace openai_agents