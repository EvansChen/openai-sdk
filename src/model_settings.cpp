#include "model_settings.h"

namespace openai_agents {

ModelSettings ModelSettings::resolve(const std::optional<ModelSettings>& override) const {
    if (!override.has_value()) {
        return *this;
    }
    
    ModelSettings result = *this;
    const ModelSettings& other = override.value();
    
    // Overlay non-null values from override
    if (other.temperature_.has_value()) result.temperature_ = other.temperature_;
    if (other.top_p_.has_value()) result.top_p_ = other.top_p_;
    if (other.frequency_penalty_.has_value()) result.frequency_penalty_ = other.frequency_penalty_;
    if (other.presence_penalty_.has_value()) result.presence_penalty_ = other.presence_penalty_;
    if (other.tool_choice_.has_value()) result.tool_choice_ = other.tool_choice_;
    if (other.parallel_tool_calls_.has_value()) result.parallel_tool_calls_ = other.parallel_tool_calls_;
    if (other.truncation_.has_value()) result.truncation_ = other.truncation_;
    if (other.max_tokens_.has_value()) result.max_tokens_ = other.max_tokens_;
    if (other.reasoning_.has_value()) result.reasoning_ = other.reasoning_;
    if (other.metadata_.has_value()) result.metadata_ = other.metadata_;
    if (other.store_.has_value()) result.store_ = other.store_;
    if (other.include_usage_.has_value()) result.include_usage_ = other.include_usage_;
    if (other.response_include_.has_value()) result.response_include_ = other.response_include_;
    if (other.extra_query_.has_value()) result.extra_query_ = other.extra_query_;
    if (other.extra_body_.has_value()) result.extra_body_ = other.extra_body_;
    if (other.extra_headers_.has_value()) result.extra_headers_ = other.extra_headers_;
    
    // Handle extra_args merging specially - merge dictionaries instead of replacing
    if (this->extra_args_.has_value() || other.extra_args_.has_value()) {
        std::map<std::string, std::string> merged_args;
        if (this->extra_args_.has_value()) {
            merged_args = this->extra_args_.value();
        }
        if (other.extra_args_.has_value()) {
            for (const auto& [key, value] : other.extra_args_.value()) {
                merged_args[key] = value;
            }
        }
        result.extra_args_ = merged_args.empty() ? std::nullopt : std::optional(merged_args);
    }
    
    return result;
}

std::map<std::string, std::variant<std::string, int, double, bool>> ModelSettings::to_json_dict() const {
    std::map<std::string, std::variant<std::string, int, double, bool>> result;
    
    if (temperature_.has_value()) {
        result["temperature"] = temperature_.value();
    }
    if (top_p_.has_value()) {
        result["top_p"] = top_p_.value();
    }
    if (frequency_penalty_.has_value()) {
        result["frequency_penalty"] = frequency_penalty_.value();
    }
    if (presence_penalty_.has_value()) {
        result["presence_penalty"] = presence_penalty_.value();
    }
    if (parallel_tool_calls_.has_value()) {
        result["parallel_tool_calls"] = parallel_tool_calls_.value();
    }
    if (truncation_.has_value()) {
        result["truncation"] = truncation_.value();
    }
    if (max_tokens_.has_value()) {
        result["max_tokens"] = max_tokens_.value();
    }
    if (store_.has_value()) {
        result["store"] = store_.value();
    }
    if (include_usage_.has_value()) {
        result["include_usage"] = include_usage_.value();
    }
    
    // Note: More complex types like tool_choice, reasoning, metadata, etc.
    // would need special handling for JSON serialization
    
    return result;
}

} // namespace openai_agents