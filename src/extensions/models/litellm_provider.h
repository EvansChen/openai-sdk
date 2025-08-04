#pragma once

/**
 * LiteLLM provider implementation for multi-provider model access
 * 
 * This provider uses LiteLLM to route requests to any supported model provider,
 * including OpenAI, Anthropic, Google, Mistral, and many others.
 * See supported models at: https://docs.litellm.ai/docs/providers
 */

#include "../../models/interface.h"
#include "litellm_model.h"
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>

namespace openai_agents {
namespace extensions {
namespace models {

/**
 * Default model to use if none specified
 */
constexpr const char* DEFAULT_MODEL = "gpt-4";

/**
 * ModelProvider implementation using LiteLLM for multi-provider access
 * 
 * This provider routes model requests through LiteLLM, which supports
 * a wide variety of model providers and handles the conversion between
 * different provider APIs and the OpenAI-compatible format.
 * 
 * @note API keys must be set via environment variables. For models requiring
 *       additional configuration (e.g., Azure API base or version), those must
 *       also be set via the environment variables that LiteLLM expects.
 * 
 * @example
 * ```cpp
 * auto provider = std::make_shared<LitellmProvider>();
 * 
 * // Use default model (gpt-4)
 * auto default_model = provider->get_model();
 * 
 * // Use specific OpenAI model
 * auto openai_model = provider->get_model("gpt-3.5-turbo");
 * 
 * // Use Anthropic model
 * auto claude_model = provider->get_model("claude-3-sonnet-20240229");
 * 
 * // Use Google model
 * auto gemini_model = provider->get_model("gemini-pro");
 * 
 * // Use in runner
 * RunConfig config;
 * config.model_provider = provider;
 * Runner::run(agent, input, config);
 * ```
 */
class LitellmProvider : public ModelProvider {
public:
    /**
     * Default constructor using environment variables for configuration
     */
    LitellmProvider() = default;

    /**
     * Constructor with explicit configuration
     * 
     * @param default_model Default model to use when none specified
     * @param global_api_key Global API key to use for all providers (optional)
     * @param global_base_url Global base URL to use for all providers (optional)
     */
    explicit LitellmProvider(
        const std::string& default_model = DEFAULT_MODEL,
        const std::optional<std::string>& global_api_key = std::nullopt,
        const std::optional<std::string>& global_base_url = std::nullopt
    );

    /**
     * Get a model instance for the specified model name
     * 
     * @param model_name Model name/identifier (e.g., "gpt-4", "claude-3-sonnet", "gemini-pro")
     *                   If null/empty, uses the default model
     * @return Shared pointer to the model instance
     * 
     * @throws std::runtime_error if the model is not supported or configuration is invalid
     */
    std::shared_ptr<Model> get_model(
        const std::optional<std::string>& model_name = std::nullopt
    ) override;

    /**
     * Get the default model name
     */
    const std::string& get_default_model() const;

    /**
     * Set the default model name
     */
    void set_default_model(const std::string& model_name);

    /**
     * Configure provider-specific settings
     * 
     * @param provider Provider name (e.g., "openai", "anthropic", "google")
     * @param api_key API key for the provider
     * @param base_url Base URL for the provider (optional)
     */
    void configure_provider(
        const std::string& provider,
        const std::string& api_key,
        const std::optional<std::string>& base_url = std::nullopt
    );

    /**
     * Remove provider configuration
     */
    void remove_provider_config(const std::string& provider);

    /**
     * Get list of configured providers
     */
    std::vector<std::string> get_configured_providers() const;

    /**
     * Check if a provider is configured
     */
    bool is_provider_configured(const std::string& provider) const;

    /**
     * Get supported providers (static list)
     */
    static std::vector<std::string> get_supported_providers();

    /**
     * Get supported models for a specific provider
     */
    static std::vector<std::string> get_supported_models(const std::string& provider);

    /**
     * Validate if a model is supported
     */
    static bool is_model_supported(const std::string& model_name);

    /**
     * Parse a model string to extract provider and model info
     */
    static std::pair<std::string, std::string> parse_model_name(const std::string& model_name);

private:
    struct ProviderConfig {
        std::string api_key;
        std::optional<std::string> base_url;
    };

    std::string default_model_;
    std::optional<std::string> global_api_key_;
    std::optional<std::string> global_base_url_;
    std::unordered_map<std::string, ProviderConfig> provider_configs_;

    /**
     * Get configuration for a specific provider
     */
    std::pair<std::optional<std::string>, std::optional<std::string>> get_provider_config(
        const std::string& provider
    ) const;

    /**
     * Load configuration from environment variables
     */
    void load_env_config();

    /**
     * Validate model configuration
     */
    void validate_model_config(const std::string& model_name) const;
};

/**
 * Factory function for creating LiteLLM provider
 */
inline std::shared_ptr<LitellmProvider> create_litellm_provider(
    const std::string& default_model = DEFAULT_MODEL,
    const std::optional<std::string>& global_api_key = std::nullopt,
    const std::optional<std::string>& global_base_url = std::nullopt
) {
    return std::make_shared<LitellmProvider>(default_model, global_api_key, global_base_url);
}

/**
 * Utility class for managing LiteLLM environment configuration
 */
class LitellmEnvironment {
public:
    /**
     * Set up environment variables for a provider
     */
    static void configure_provider_env(
        const std::string& provider,
        const std::string& api_key,
        const std::optional<std::string>& base_url = std::nullopt
    );

    /**
     * Get environment variable name for provider API key
     */
    static std::string get_api_key_env_name(const std::string& provider);

    /**
     * Get environment variable name for provider base URL
     */
    static std::string get_base_url_env_name(const std::string& provider);

    /**
     * Load all provider configurations from environment
     */
    static std::unordered_map<std::string, std::pair<std::string, std::optional<std::string>>> 
    load_all_provider_configs();

    /**
     * Check if a provider is configured in environment
     */
    static bool is_provider_configured_in_env(const std::string& provider);

    /**
     * Get list of providers configured in environment
     */
    static std::vector<std::string> get_env_configured_providers();

private:
    static const std::unordered_map<std::string, std::pair<std::string, std::string>> env_mapping_;
};

/**
 * Configuration builder for LiteLLM provider
 */
class LitellmProviderBuilder {
public:
    LitellmProviderBuilder() = default;

    /**
     * Set default model
     */
    LitellmProviderBuilder& default_model(const std::string& model);

    /**
     * Set global API key
     */
    LitellmProviderBuilder& global_api_key(const std::string& api_key);

    /**
     * Set global base URL
     */
    LitellmProviderBuilder& global_base_url(const std::string& base_url);

    /**
     * Configure specific provider
     */
    LitellmProviderBuilder& configure_provider(
        const std::string& provider,
        const std::string& api_key,
        const std::optional<std::string>& base_url = std::nullopt
    );

    /**
     * Load configuration from environment
     */
    LitellmProviderBuilder& from_environment();

    /**
     * Build the provider
     */
    std::shared_ptr<LitellmProvider> build();

private:
    std::string default_model_ = DEFAULT_MODEL;
    std::optional<std::string> global_api_key_;
    std::optional<std::string> global_base_url_;
    std::unordered_map<std::string, std::pair<std::string, std::optional<std::string>>> provider_configs_;
    bool load_from_env_ = false;
};

} // namespace models
} // namespace extensions
} // namespace openai_agents