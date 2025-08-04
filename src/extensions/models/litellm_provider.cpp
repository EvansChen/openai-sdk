#include "litellm_provider.h"
#include "../../util/_error_tracing.h"
#include <stdexcept>
#include <algorithm>
#include <cstdlib>
#include <cctype>

namespace openai_agents {
namespace extensions {
namespace models {

// Static data for provider mappings
const std::unordered_map<std::string, std::pair<std::string, std::string>> 
LitellmEnvironment::env_mapping_ = {
    {"openai", {"OPENAI_API_KEY", "OPENAI_API_BASE"}},
    {"anthropic", {"ANTHROPIC_API_KEY", "ANTHROPIC_API_BASE"}},
    {"google", {"GOOGLE_API_KEY", "GOOGLE_API_BASE"}},
    {"cohere", {"COHERE_API_KEY", "COHERE_API_BASE"}},
    {"huggingface", {"HUGGINGFACE_API_KEY", "HUGGINGFACE_API_BASE"}},
    {"mistral", {"MISTRAL_API_KEY", "MISTRAL_API_BASE"}},
    {"together", {"TOGETHERAI_API_KEY", "TOGETHERAI_API_BASE"}},
    {"fireworks", {"FIREWORKS_API_KEY", "FIREWORKS_API_BASE"}},
    {"replicate", {"REPLICATE_API_TOKEN", "REPLICATE_API_BASE"}},
    {"palm", {"PALM_API_KEY", "PALM_API_BASE"}},
    {"azure", {"AZURE_API_KEY", "AZURE_API_BASE"}},
    {"bedrock", {"AWS_ACCESS_KEY_ID", "BEDROCK_API_BASE"}},
    {"vertex", {"GOOGLE_APPLICATION_CREDENTIALS", "VERTEX_API_BASE"}},
    {"perplexity", {"PERPLEXITYAI_API_KEY", "PERPLEXITYAI_API_BASE"}},
    {"groq", {"GROQ_API_KEY", "GROQ_API_BASE"}},
    {"deepinfra", {"DEEPINFRA_API_KEY", "DEEPINFRA_API_BASE"}},
    {"ai21", {"AI21_API_KEY", "AI21_API_BASE"}},
    {"nlp_cloud", {"NLP_CLOUD_API_KEY", "NLP_CLOUD_API_BASE"}},
    {"aleph_alpha", {"ALEPH_ALPHA_API_KEY", "ALEPH_ALPHA_API_BASE"}},
    {"petals", {"PETALS_API_KEY", "PETALS_API_BASE"}}
};

// LitellmProvider implementation
LitellmProvider::LitellmProvider(
    const std::string& default_model,
    const std::optional<std::string>& global_api_key,
    const std::optional<std::string>& global_base_url
) : default_model_(default_model),
    global_api_key_(global_api_key),
    global_base_url_(global_base_url) {
    load_env_config();
}

std::shared_ptr<Model> LitellmProvider::get_model(
    const std::optional<std::string>& model_name
) {
    std::string actual_model = model_name.value_or(default_model_);
    
    if (actual_model.empty()) {
        actual_model = default_model_;
    }

    validate_model_config(actual_model);

    auto [provider, model] = parse_model_name(actual_model);
    auto [api_key, base_url] = get_provider_config(provider);

    return std::make_shared<LitellmModel>(
        actual_model,
        api_key,
        base_url
    );
}

const std::string& LitellmProvider::get_default_model() const {
    return default_model_;
}

void LitellmProvider::set_default_model(const std::string& model_name) {
    if (model_name.empty()) {
        throw std::invalid_argument("Model name cannot be empty");
    }
    default_model_ = model_name;
}

void LitellmProvider::configure_provider(
    const std::string& provider,
    const std::string& api_key,
    const std::optional<std::string>& base_url
) {
    if (provider.empty()) {
        throw std::invalid_argument("Provider name cannot be empty");
    }
    if (api_key.empty()) {
        throw std::invalid_argument("API key cannot be empty");
    }

    provider_configs_[provider] = {api_key, base_url};
}

void LitellmProvider::remove_provider_config(const std::string& provider) {
    provider_configs_.erase(provider);
}

std::vector<std::string> LitellmProvider::get_configured_providers() const {
    std::vector<std::string> providers;
    providers.reserve(provider_configs_.size());
    
    for (const auto& [provider, config] : provider_configs_) {
        providers.push_back(provider);
    }
    
    return providers;
}

bool LitellmProvider::is_provider_configured(const std::string& provider) const {
    return provider_configs_.find(provider) != provider_configs_.end() ||
           LitellmEnvironment::is_provider_configured_in_env(provider);
}

std::vector<std::string> LitellmProvider::get_supported_providers() {
    return {
        "openai", "anthropic", "google", "cohere", "huggingface",
        "mistral", "together", "fireworks", "replicate", "palm",
        "azure", "bedrock", "vertex", "perplexity", "groq",
        "deepinfra", "ai21", "nlp_cloud", "aleph_alpha", "petals"
    };
}

std::vector<std::string> LitellmProvider::get_supported_models(const std::string& provider) {
    // Return commonly supported models for each provider
    // This is a simplified list - LiteLLM supports many more models
    static const std::unordered_map<std::string, std::vector<std::string>> provider_models = {
        {"openai", {
            "gpt-4", "gpt-4-turbo", "gpt-4o", "gpt-4o-mini",
            "gpt-3.5-turbo", "gpt-3.5-turbo-16k",
            "text-davinci-003", "text-curie-001"
        }},
        {"anthropic", {
            "claude-3-opus-20240229", "claude-3-sonnet-20240229", "claude-3-haiku-20240307",
            "claude-2.1", "claude-2.0", "claude-instant-1.2"
        }},
        {"google", {
            "gemini-pro", "gemini-pro-vision", "gemini-1.5-pro", "gemini-1.5-flash",
            "chat-bison", "text-bison", "codechat-bison"
        }},
        {"cohere", {
            "command", "command-light", "command-nightly", "command-r", "command-r-plus"
        }},
        {"mistral", {
            "mistral-tiny", "mistral-small", "mistral-medium", "mistral-large",
            "mixtral-8x7b-instruct", "mistral-7b-instruct"
        }},
        {"huggingface", {
            "meta-llama/Llama-2-7b-chat-hf", "meta-llama/Llama-2-13b-chat-hf",
            "meta-llama/Llama-2-70b-chat-hf", "codellama/CodeLlama-34b-Instruct-hf"
        }},
        {"together", {
            "together_ai/llama-2-7b-chat", "together_ai/llama-2-13b-chat",
            "together_ai/llama-2-70b-chat", "together_ai/falcon-40b-instruct"
        }},
        {"fireworks", {
            "fireworks_ai/llama-v2-7b-chat", "fireworks_ai/llama-v2-13b-chat",
            "fireworks_ai/llama-v2-70b-chat", "fireworks_ai/mixtral-8x7b-instruct"
        }}
    };

    auto it = provider_models.find(provider);
    if (it != provider_models.end()) {
        return it->second;
    }
    return {};
}

bool LitellmProvider::is_model_supported(const std::string& model_name) {
    auto [provider, model] = parse_model_name(model_name);
    auto supported_providers = get_supported_providers();
    
    return std::find(supported_providers.begin(), supported_providers.end(), provider) 
           != supported_providers.end();
}

std::pair<std::string, std::string> LitellmProvider::parse_model_name(const std::string& model_name) {
    // Handle provider/model format (e.g., "openai/gpt-4")
    auto slash_pos = model_name.find('/');
    if (slash_pos != std::string::npos) {
        return {model_name.substr(0, slash_pos), model_name.substr(slash_pos + 1)};
    }

    // Infer provider from model name patterns
    std::string lower_model = model_name;
    std::transform(lower_model.begin(), lower_model.end(), lower_model.begin(), ::tolower);

    if (lower_model.find("gpt") != std::string::npos || 
        lower_model.find("davinci") != std::string::npos ||
        lower_model.find("curie") != std::string::npos ||
        lower_model.find("babbage") != std::string::npos ||
        lower_model.find("ada") != std::string::npos) {
        return {"openai", model_name};
    }
    
    if (lower_model.find("claude") != std::string::npos) {
        return {"anthropic", model_name};
    }
    
    if (lower_model.find("gemini") != std::string::npos ||
        lower_model.find("bison") != std::string::npos ||
        lower_model.find("palm") != std::string::npos) {
        return {"google", model_name};
    }
    
    if (lower_model.find("command") != std::string::npos) {
        return {"cohere", model_name};
    }
    
    if (lower_model.find("mistral") != std::string::npos ||
        lower_model.find("mixtral") != std::string::npos) {
        return {"mistral", model_name};
    }

    // Default to openai if no pattern matches
    return {"openai", model_name};
}

std::pair<std::optional<std::string>, std::optional<std::string>> 
LitellmProvider::get_provider_config(const std::string& provider) const {
    // First check explicit provider configurations
    auto it = provider_configs_.find(provider);
    if (it != provider_configs_.end()) {
        return {it->second.api_key, it->second.base_url};
    }

    // Then check global configuration
    if (global_api_key_.has_value()) {
        return {global_api_key_, global_base_url_};
    }

    // Finally check environment variables
    auto env_it = LitellmEnvironment::env_mapping_.find(provider);
    if (env_it != LitellmEnvironment::env_mapping_.end()) {
        auto api_key_env = std::getenv(env_it->second.first.c_str());
        auto base_url_env = std::getenv(env_it->second.second.c_str());
        
        std::optional<std::string> api_key = api_key_env ? std::optional<std::string>(api_key_env) : std::nullopt;
        std::optional<std::string> base_url = base_url_env ? std::optional<std::string>(base_url_env) : std::nullopt;
        
        return {api_key, base_url};
    }

    return {std::nullopt, std::nullopt};
}

void LitellmProvider::load_env_config() {
    // Load provider configurations from environment variables
    for (const auto& [provider, env_vars] : LitellmEnvironment::env_mapping_) {
        auto api_key_env = std::getenv(env_vars.first.c_str());
        if (api_key_env) {
            auto base_url_env = std::getenv(env_vars.second.c_str());
            std::optional<std::string> base_url = base_url_env ? 
                std::optional<std::string>(base_url_env) : std::nullopt;
            
            provider_configs_[provider] = {api_key_env, base_url};
        }
    }
}

void LitellmProvider::validate_model_config(const std::string& model_name) const {
    if (model_name.empty()) {
        throw std::invalid_argument("Model name cannot be empty");
    }

    auto [provider, model] = parse_model_name(model_name);
    auto [api_key, base_url] = get_provider_config(provider);

    if (!api_key.has_value() || api_key->empty()) {
        throw std::runtime_error(
            "No API key configured for provider '" + provider + 
            "'. Set the appropriate environment variable or configure the provider explicitly."
        );
    }
}

// LitellmEnvironment implementation
void LitellmEnvironment::configure_provider_env(
    const std::string& provider,
    const std::string& api_key,
    const std::optional<std::string>& base_url
) {
    auto it = env_mapping_.find(provider);
    if (it == env_mapping_.end()) {
        throw std::invalid_argument("Unknown provider: " + provider);
    }

    // Set API key environment variable
#ifdef _WIN32
    _putenv_s(it->second.first.c_str(), api_key.c_str());
#else
    setenv(it->second.first.c_str(), api_key.c_str(), 1);
#endif

    // Set base URL if provided
    if (base_url.has_value()) {
#ifdef _WIN32
        _putenv_s(it->second.second.c_str(), base_url->c_str());
#else
        setenv(it->second.second.c_str(), base_url->c_str(), 1);
#endif
    }
}

std::string LitellmEnvironment::get_api_key_env_name(const std::string& provider) {
    auto it = env_mapping_.find(provider);
    if (it != env_mapping_.end()) {
        return it->second.first;
    }
    return "";
}

std::string LitellmEnvironment::get_base_url_env_name(const std::string& provider) {
    auto it = env_mapping_.find(provider);
    if (it != env_mapping_.end()) {
        return it->second.second;
    }
    return "";
}

std::unordered_map<std::string, std::pair<std::string, std::optional<std::string>>> 
LitellmEnvironment::load_all_provider_configs() {
    std::unordered_map<std::string, std::pair<std::string, std::optional<std::string>>> configs;
    
    for (const auto& [provider, env_vars] : env_mapping_) {
        auto api_key_env = std::getenv(env_vars.first.c_str());
        if (api_key_env) {
            auto base_url_env = std::getenv(env_vars.second.c_str());
            std::optional<std::string> base_url = base_url_env ? 
                std::optional<std::string>(base_url_env) : std::nullopt;
            
            configs[provider] = {api_key_env, base_url};
        }
    }
    
    return configs;
}

bool LitellmEnvironment::is_provider_configured_in_env(const std::string& provider) {
    auto it = env_mapping_.find(provider);
    if (it == env_mapping_.end()) {
        return false;
    }
    
    auto api_key_env = std::getenv(it->second.first.c_str());
    return api_key_env != nullptr && std::string(api_key_env).length() > 0;
}

std::vector<std::string> LitellmEnvironment::get_env_configured_providers() {
    std::vector<std::string> providers;
    
    for (const auto& [provider, env_vars] : env_mapping_) {
        if (is_provider_configured_in_env(provider)) {
            providers.push_back(provider);
        }
    }
    
    return providers;
}

// LitellmProviderBuilder implementation
LitellmProviderBuilder& LitellmProviderBuilder::default_model(const std::string& model) {
    default_model_ = model;
    return *this;
}

LitellmProviderBuilder& LitellmProviderBuilder::global_api_key(const std::string& api_key) {
    global_api_key_ = api_key;
    return *this;
}

LitellmProviderBuilder& LitellmProviderBuilder::global_base_url(const std::string& base_url) {
    global_base_url_ = base_url;
    return *this;
}

LitellmProviderBuilder& LitellmProviderBuilder::configure_provider(
    const std::string& provider,
    const std::string& api_key,
    const std::optional<std::string>& base_url
) {
    provider_configs_[provider] = {api_key, base_url};
    return *this;
}

LitellmProviderBuilder& LitellmProviderBuilder::from_environment() {
    load_from_env_ = true;
    return *this;
}

std::shared_ptr<LitellmProvider> LitellmProviderBuilder::build() {
    auto provider = std::make_shared<LitellmProvider>(
        default_model_, 
        global_api_key_, 
        global_base_url_
    );

    // Configure explicit providers
    for (const auto& [name, config] : provider_configs_) {
        provider->configure_provider(name, config.first, config.second);
    }

    return provider;
}

} // namespace models
} // namespace extensions
} // namespace openai_agents