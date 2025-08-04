#pragma once

/**
 * Extensions Models Module
 * 
 * This module provides additional model implementations and providers
 * for the OpenAI Agents framework, extending beyond the core model interface.
 * 
 * Current implementations:
 * - LiteLLM integration for multi-provider model access
 * 
 * The LiteLLM integration allows the framework to work with models from
 * any provider supported by LiteLLM, including:
 * - OpenAI (GPT-4, GPT-3.5, etc.)
 * - Anthropic (Claude 3, Claude 2, etc.)
 * - Google (Gemini Pro, PaLM, etc.)
 * - Cohere (Command models)
 * - Mistral AI (Mistral, Mixtral models)
 * - Hugging Face models
 * - And many more...
 * 
 * @example Basic Usage
 * ```cpp
 * #include "extensions/models/litellm_provider.h"
 * 
 * // Create provider with default settings
 * auto provider = create_litellm_provider();
 * 
 * // Use in agent configuration
 * RunConfig config;
 * config.model_provider = provider;
 * 
 * // Get specific models
 * auto gpt4 = provider->get_model("gpt-4");
 * auto claude = provider->get_model("claude-3-sonnet-20240229");
 * auto gemini = provider->get_model("gemini-pro");
 * ```
 * 
 * @example Provider Configuration
 * ```cpp
 * // Builder pattern for complex configuration
 * auto provider = LitellmProviderBuilder()
 *     .default_model("gpt-4")
 *     .configure_provider("openai", "sk-...", "https://api.openai.com/v1")
 *     .configure_provider("anthropic", "sk-ant-...", std::nullopt)
 *     .from_environment()
 *     .build();
 * ```
 * 
 * @example Environment Configuration
 * ```cpp
 * // Configure via environment variables
 * LitellmEnvironment::configure_provider_env(
 *     "openai", 
 *     "sk-your-api-key",
 *     "https://api.openai.com/v1"
 * );
 * 
 * auto provider = create_litellm_provider();
 * // Provider will automatically use environment configuration
 * ```
 */

// LiteLLM Integration
#include "litellm_model.h"
#include "litellm_provider.h"

namespace openai_agents {
namespace extensions {

/**
 * Models namespace contains additional model implementations
 * and providers for extended functionality
 */
namespace models {

/**
 * Create a default LiteLLM provider with environment configuration
 * 
 * This is the simplest way to get started with multi-provider model access.
 * The provider will automatically detect and use API keys from environment
 * variables for all supported providers.
 * 
 * @param default_model Default model to use (defaults to "gpt-4")
 * @return Configured LiteLLM provider
 */
inline std::shared_ptr<LitellmProvider> create_default_provider(
    const std::string& default_model = "gpt-4"
) {
    return create_litellm_provider(default_model);
}

/**
 * Create a provider configured for OpenAI only
 * 
 * @param api_key OpenAI API key
 * @param default_model Default OpenAI model to use
 * @param base_url Custom base URL (optional)
 * @return Configured provider for OpenAI
 */
inline std::shared_ptr<LitellmProvider> create_openai_provider(
    const std::string& api_key,
    const std::string& default_model = "gpt-4",
    const std::optional<std::string>& base_url = std::nullopt
) {
    return LitellmProviderBuilder()
        .default_model(default_model)
        .configure_provider("openai", api_key, base_url)
        .build();
}

/**
 * Create a provider configured for Anthropic only
 * 
 * @param api_key Anthropic API key
 * @param default_model Default Anthropic model to use
 * @param base_url Custom base URL (optional)
 * @return Configured provider for Anthropic
 */
inline std::shared_ptr<LitellmProvider> create_anthropic_provider(
    const std::string& api_key,
    const std::string& default_model = "claude-3-sonnet-20240229",
    const std::optional<std::string>& base_url = std::nullopt
) {
    return LitellmProviderBuilder()
        .default_model(default_model)
        .configure_provider("anthropic", api_key, base_url)
        .build();
}

/**
 * Create a provider configured for Google only
 * 
 * @param api_key Google API key
 * @param default_model Default Google model to use
 * @param base_url Custom base URL (optional)
 * @return Configured provider for Google
 */
inline std::shared_ptr<LitellmProvider> create_google_provider(
    const std::string& api_key,
    const std::string& default_model = "gemini-pro",
    const std::optional<std::string>& base_url = std::nullopt
) {
    return LitellmProviderBuilder()
        .default_model(default_model)
        .configure_provider("google", api_key, base_url)
        .build();
}

/**
 * Create a multi-provider setup with common providers
 * 
 * @param openai_key OpenAI API key
 * @param anthropic_key Anthropic API key
 * @param google_key Google API key
 * @param default_model Default model to use
 * @return Configured multi-provider setup
 */
inline std::shared_ptr<LitellmProvider> create_multi_provider(
    const std::string& openai_key,
    const std::string& anthropic_key,
    const std::string& google_key,
    const std::string& default_model = "gpt-4"
) {
    return LitellmProviderBuilder()
        .default_model(default_model)
        .configure_provider("openai", openai_key)
        .configure_provider("anthropic", anthropic_key)
        .configure_provider("google", google_key)
        .build();
}

/**
 * Get list of all supported providers
 */
inline std::vector<std::string> get_supported_providers() {
    return LitellmProvider::get_supported_providers();
}

/**
 * Get list of supported models for a provider
 */
inline std::vector<std::string> get_supported_models(const std::string& provider) {
    return LitellmProvider::get_supported_models(provider);
}

/**
 * Check if a model is supported
 */
inline bool is_model_supported(const std::string& model_name) {
    return LitellmProvider::is_model_supported(model_name);
}

/**
 * Parse model name to extract provider and model info
 */
inline std::pair<std::string, std::string> parse_model_name(const std::string& model_name) {
    return LitellmProvider::parse_model_name(model_name);
}

} // namespace models
} // namespace extensions
} // namespace openai_agents