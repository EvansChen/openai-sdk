#pragma once

/**
 * JSON utilities for the agents framework
 */

#include <string>
#include <map>
#include <vector>
#include <any>
#include <memory>
#include <optional>

namespace openai_agents {
namespace util {

// JSON value types
enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

// JSON value container
class JsonValue {
private:
    JsonType type_;
    std::any value_;

public:
    JsonValue() : type_(JsonType::Null) {}
    JsonValue(bool value) : type_(JsonType::Boolean), value_(value) {}
    JsonValue(int value) : type_(JsonType::Number), value_(static_cast<double>(value)) {}
    JsonValue(double value) : type_(JsonType::Number), value_(value) {}
    JsonValue(const std::string& value) : type_(JsonType::String), value_(value) {}
    JsonValue(const char* value) : type_(JsonType::String), value_(std::string(value)) {}
    JsonValue(const std::vector<JsonValue>& value) : type_(JsonType::Array), value_(value) {}
    JsonValue(const std::map<std::string, JsonValue>& value) : type_(JsonType::Object), value_(value) {}
    
    // Type checking
    JsonType get_type() const { return type_; }
    bool is_null() const { return type_ == JsonType::Null; }
    bool is_boolean() const { return type_ == JsonType::Boolean; }
    bool is_number() const { return type_ == JsonType::Number; }
    bool is_string() const { return type_ == JsonType::String; }
    bool is_array() const { return type_ == JsonType::Array; }
    bool is_object() const { return type_ == JsonType::Object; }
    
    // Value access (with type checking)
    bool as_boolean() const;
    double as_number() const;
    int as_int() const;
    std::string as_string() const;
    std::vector<JsonValue> as_array() const;
    std::map<std::string, JsonValue> as_object() const;
    
    // Safe value access (returns default if type mismatch)
    bool as_boolean_or(bool default_value) const;
    double as_number_or(double default_value) const;
    int as_int_or(int default_value) const;
    std::string as_string_or(const std::string& default_value) const;
    
    // Array operations
    size_t array_size() const;
    JsonValue array_at(size_t index) const;
    void array_push(const JsonValue& value);
    
    // Object operations
    size_t object_size() const;
    bool object_has(const std::string& key) const;
    JsonValue object_get(const std::string& key) const;
    JsonValue object_get_or(const std::string& key, const JsonValue& default_value) const;
    void object_set(const std::string& key, const JsonValue& value);
    void object_remove(const std::string& key);
    std::vector<std::string> object_keys() const;
    
    // Operators for object access
    JsonValue& operator[](const std::string& key);
    const JsonValue& operator[](const std::string& key) const;
    JsonValue& operator[](size_t index);
    const JsonValue& operator[](size_t index) const;
    
    // Serialization
    std::string to_string(bool pretty = false, int indent = 0) const;
    
    // Equality
    bool operator==(const JsonValue& other) const;
    bool operator!=(const JsonValue& other) const { return !(*this == other); }
};

// JSON parser
class JsonParser {
public:
    static JsonValue parse(const std::string& json_str);
    static JsonValue parse_file(const std::string& filename);
    
    // Error handling
    struct ParseError {
        std::string message;
        size_t line;
        size_t column;
        size_t position;
    };
    
    static std::optional<ParseError> get_last_error();

private:
    static JsonValue parse_value(const std::string& json, size_t& pos);
    static JsonValue parse_object(const std::string& json, size_t& pos);
    static JsonValue parse_array(const std::string& json, size_t& pos);
    static JsonValue parse_string(const std::string& json, size_t& pos);
    static JsonValue parse_number(const std::string& json, size_t& pos);
    static JsonValue parse_literal(const std::string& json, size_t& pos);
    
    static void skip_whitespace(const std::string& json, size_t& pos);
    static void expect_char(const std::string& json, size_t& pos, char expected);
    static std::string unescape_string(const std::string& str);
    
    static thread_local std::optional<ParseError> last_error_;
};

// JSON builder for easy construction
class JsonBuilder {
private:
    JsonValue root_;

public:
    JsonBuilder() : root_(std::map<std::string, JsonValue>{}) {}
    JsonBuilder(const JsonValue& root) : root_(root) {}
    
    // Object building
    JsonBuilder& set(const std::string& key, const JsonValue& value);
    JsonBuilder& set(const std::string& key, bool value);
    JsonBuilder& set(const std::string& key, int value);
    JsonBuilder& set(const std::string& key, double value);
    JsonBuilder& set(const std::string& key, const std::string& value);
    JsonBuilder& set(const std::string& key, const char* value);
    
    // Nested object building
    JsonBuilder& set_object(const std::string& key);
    JsonBuilder& set_array(const std::string& key);
    
    // Array building
    JsonBuilder& push(const JsonValue& value);
    JsonBuilder& push(bool value);
    JsonBuilder& push(int value);
    JsonBuilder& push(double value);
    JsonBuilder& push(const std::string& value);
    JsonBuilder& push(const char* value);
    
    // Build result
    JsonValue build() const { return root_; }
    std::string to_string(bool pretty = false) const { return root_.to_string(pretty); }
};

// Utility functions
class JsonUtils {
public:
    // Type conversion helpers
    static JsonValue from_any(const std::any& value);
    static std::any to_any(const JsonValue& json);
    
    // Map conversion
    static JsonValue from_map(const std::map<std::string, std::any>& map);
    static std::map<std::string, std::any> to_map(const JsonValue& json);
    
    // Vector conversion
    static JsonValue from_vector(const std::vector<std::any>& vec);
    static std::vector<std::any> to_vector(const JsonValue& json);
    
    // Deep operations
    static JsonValue deep_copy(const JsonValue& json);
    static JsonValue deep_merge(const JsonValue& base, const JsonValue& overlay);
    
    // Path operations
    static JsonValue get_at_path(const JsonValue& json, const std::string& path);
    static void set_at_path(JsonValue& json, const std::string& path, const JsonValue& value);
    static bool has_path(const JsonValue& json, const std::string& path);
    
    // Validation
    static bool is_valid_json(const std::string& json_str);
    static std::vector<std::string> validate_schema(const JsonValue& json, const JsonValue& schema);
    
    // Pretty printing
    static std::string pretty_print(const JsonValue& json, int indent_size = 2);
    static std::string minify(const JsonValue& json);
    
    // Diff
    static JsonValue diff(const JsonValue& old_json, const JsonValue& new_json);
    static JsonValue patch(const JsonValue& json, const JsonValue& patch);
};

// JSON serializable interface
class JsonSerializable {
public:
    virtual ~JsonSerializable() = default;
    virtual JsonValue to_json() const = 0;
    virtual void from_json(const JsonValue& json) = 0;
};

// Template helpers for easy serialization
template<typename T>
JsonValue serialize_vector(const std::vector<T>& vec) {
    std::vector<JsonValue> json_array;
    json_array.reserve(vec.size());
    for (const auto& item : vec) {
        if constexpr (std::is_base_of_v<JsonSerializable, T>) {
            json_array.push_back(item.to_json());
        } else {
            json_array.push_back(JsonUtils::from_any(std::any(item)));
        }
    }
    return JsonValue(json_array);
}

template<typename T>
std::vector<T> deserialize_vector(const JsonValue& json) {
    if (!json.is_array()) {
        throw std::runtime_error("Expected JSON array");
    }
    
    auto json_array = json.as_array();
    std::vector<T> result;
    result.reserve(json_array.size());
    
    for (const auto& item : json_array) {
        if constexpr (std::is_base_of_v<JsonSerializable, T>) {
            T obj;
            obj.from_json(item);
            result.push_back(std::move(obj));
        } else {
            auto any_val = JsonUtils::to_any(item);
            result.push_back(std::any_cast<T>(any_val));
        }
    }
    
    return result;
}

// JSON literals (C++11 user-defined literals)
namespace literals {
    JsonValue operator""_json(const char* str, size_t len);
}

} // namespace util
} // namespace openai_agents