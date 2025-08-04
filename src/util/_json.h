#pragma once

/**
 * JSON Validation Utilities for OpenAI Agents Framework
 * 
 * This module provides utilities for validating JSON strings against
 * specific types and schemas, with support for partial validation
 * and error tracing.
 */

#include "_error_tracing.h"
#include "../exceptions.h"
#include "../tracing/spans.h"
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <type_traits>
#include <stdexcept>

namespace openai_agents {
namespace util {

/**
 * JSON validation modes
 */
enum class ValidationMode {
    STRICT,              // Strict validation - all fields must be present and valid
    PARTIAL,             // Partial validation - missing fields are allowed
    TRAILING_STRINGS     // Allow trailing strings (incomplete JSON)
};

/**
 * Type adapter interface for JSON validation
 * 
 * This is similar to Pydantic's TypeAdapter but for C++ types.
 * It provides a unified interface for validating JSON against C++ types.
 * 
 * @tparam T The C++ type to validate against
 */
template<typename T>
class TypeAdapter {
public:
    virtual ~TypeAdapter() = default;
    
    /**
     * Validate JSON string and convert to type T
     * 
     * @param json_str The JSON string to validate
     * @param mode The validation mode to use
     * @return The validated and converted object
     * @throws ValidationError if validation fails
     */
    virtual T validate_json(const std::string& json_str, ValidationMode mode = ValidationMode::STRICT) = 0;
    
    /**
     * Validate JSON object and convert to type T
     * 
     * @param json_obj The JSON object to validate
     * @param mode The validation mode to use
     * @return The validated and converted object
     * @throws ValidationError if validation fails
     */
    virtual T validate_json(const nlohmann::json& json_obj, ValidationMode mode = ValidationMode::STRICT) = 0;
    
    /**
     * Get the type name for error messages
     */
    virtual std::string get_type_name() const = 0;
};

/**
 * Exception thrown when JSON validation fails
 */
class ValidationError : public std::runtime_error {
public:
    ValidationError(const std::string& message) : std::runtime_error(message) {}
    ValidationError(const std::string& message, const nlohmann::json& invalid_data)
        : std::runtime_error(message), invalid_data_(invalid_data) {}
    
    const nlohmann::json& get_invalid_data() const { return invalid_data_; }
    
private:
    nlohmann::json invalid_data_;
};

/**
 * Default type adapter for types that can be directly constructed from JSON
 * 
 * This adapter works with types that have:
 * - A constructor that takes nlohmann::json
 * - from_json() function
 * - Custom conversion logic
 */
template<typename T>
class DefaultTypeAdapter : public TypeAdapter<T> {
public:
    T validate_json(const std::string& json_str, ValidationMode mode = ValidationMode::STRICT) override {
        try {
            nlohmann::json json_obj;
            
            if (mode == ValidationMode::TRAILING_STRINGS) {
                // For trailing strings mode, we try to parse as much as possible
                json_obj = nlohmann::json::parse(json_str, nullptr, false, true);
            } else {
                json_obj = nlohmann::json::parse(json_str);
            }
            
            return validate_json(json_obj, mode);
        } catch (const nlohmann::json::parse_error& e) {
            throw ValidationError("JSON parse error: " + std::string(e.what()));
        }
    }
    
    T validate_json(const nlohmann::json& json_obj, ValidationMode mode = ValidationMode::STRICT) override {
        try {
            if constexpr (std::is_constructible_v<T, nlohmann::json>) {
                return T(json_obj);
            } else if constexpr (std::is_default_constructible_v<T>) {
                T result;
                from_json(json_obj, result);
                return result;
            } else {
                // Try nlohmann::json's automatic conversion
                return json_obj.get<T>();
            }
        } catch (const nlohmann::json::exception& e) {
            throw ValidationError("JSON validation error: " + std::string(e.what()), json_obj);
        }
    }
    
    std::string get_type_name() const override {
        return typeid(T).name();
    }
};

/**
 * Create a type adapter for type T
 * 
 * @tparam T The type to create an adapter for
 * @return A unique pointer to the type adapter
 */
template<typename T>
std::unique_ptr<TypeAdapter<T>> create_type_adapter() {
    return std::make_unique<DefaultTypeAdapter<T>>();
}

/**
 * Validate JSON string against a type with automatic error tracing
 * 
 * This is the main function for JSON validation, equivalent to Python's
 * validate_json function. It automatically attaches validation errors
 * to the current tracing span.
 * 
 * @tparam T The type to validate against
 * @param json_str The JSON string to validate
 * @param type_adapter The type adapter to use for validation
 * @param partial Whether to allow partial validation
 * @return The validated object of type T
 * @throws ModelBehaviorError if validation fails
 */
template<typename T>
T validate_json(
    const std::string& json_str,
    TypeAdapter<T>& type_adapter,
    bool partial = false
) {
    ValidationMode mode = partial ? ValidationMode::TRAILING_STRINGS : ValidationMode::STRICT;
    
    try {
        return type_adapter.validate_json(json_str, mode);
    } catch (const ValidationError& e) {
        // Attach error to current span
        tracing::SpanError span_error{
            .message = "Invalid JSON provided",
            .data = {
                {"json_string", json_str},
                {"type_name", type_adapter.get_type_name()},
                {"validation_error", e.what()}
            }
        };
        attach_error_to_current_span(span_error);
        
        // Convert to ModelBehaviorError
        std::string error_msg = "Invalid JSON when parsing '" + json_str + 
                               "' for " + type_adapter.get_type_name() + "; " + e.what();
        throw ModelBehaviorError(error_msg);
    }
}

/**
 * Convenience function for validating JSON with automatic type adapter creation
 * 
 * @tparam T The type to validate against
 * @param json_str The JSON string to validate
 * @param partial Whether to allow partial validation
 * @return The validated object of type T
 */
template<typename T>
T validate_json(const std::string& json_str, bool partial = false) {
    auto adapter = create_type_adapter<T>();
    return validate_json(json_str, *adapter, partial);
}

/**
 * Validate JSON object (already parsed) against a type
 * 
 * @tparam T The type to validate against
 * @param json_obj The JSON object to validate
 * @param type_adapter The type adapter to use for validation
 * @param partial Whether to allow partial validation
 * @return The validated object of type T
 */
template<typename T>
T validate_json_object(
    const nlohmann::json& json_obj,
    TypeAdapter<T>& type_adapter,
    bool partial = false
) {
    ValidationMode mode = partial ? ValidationMode::PARTIAL : ValidationMode::STRICT;
    
    try {
        return type_adapter.validate_json(json_obj, mode);
    } catch (const ValidationError& e) {
        // Attach error to current span
        tracing::SpanError span_error{
            .message = "Invalid JSON object provided",
            .data = {
                {"json_object", json_obj.dump()},
                {"type_name", type_adapter.get_type_name()},
                {"validation_error", e.what()}
            }
        };
        attach_error_to_current_span(span_error);
        
        // Convert to ModelBehaviorError
        std::string error_msg = "Invalid JSON object when validating for " + 
                               type_adapter.get_type_name() + "; " + e.what();
        throw ModelBehaviorError(error_msg);
    }
}

/**
 * Convenience function for validating JSON object with automatic type adapter creation
 * 
 * @tparam T The type to validate against
 * @param json_obj The JSON object to validate
 * @param partial Whether to allow partial validation
 * @return The validated object of type T
 */
template<typename T>
T validate_json_object(const nlohmann::json& json_obj, bool partial = false) {
    auto adapter = create_type_adapter<T>();
    return validate_json_object(json_obj, *adapter, partial);
}

/**
 * Try to parse JSON with fallback on error
 * 
 * @param json_str The JSON string to parse
 * @param fallback_value The value to return if parsing fails
 * @return The parsed JSON object or the fallback value
 */
nlohmann::json try_parse_json(const std::string& json_str, const nlohmann::json& fallback_value = nlohmann::json::object());

/**
 * Check if a string contains valid JSON
 * 
 * @param json_str The string to check
 * @return True if the string is valid JSON, false otherwise
 */
bool is_valid_json(const std::string& json_str);

/**
 * Extract partial JSON from a string that may contain trailing text
 * 
 * This function attempts to extract valid JSON from the beginning of a string,
 * useful for handling streaming JSON or partial responses.
 * 
 * @param text The text that may contain JSON
 * @return The extracted JSON object, or null if no valid JSON found
 */
nlohmann::json extract_partial_json(const std::string& text);

} // namespace util
} // namespace openai_agents