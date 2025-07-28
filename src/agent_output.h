#pragma once

/**
 * Agent output schema definitions
 */

namespace openai_agents {

/**
 * Base class for agent output schemas
 */
class AgentOutputSchemaBase {
public:
    virtual ~AgentOutputSchemaBase() = default;
    virtual std::string get_schema_type() const = 0;
    virtual bool validate(const std::any& output) const = 0;
};

/**
 * Generic agent output schema
 */
template<typename OutputType>
class AgentOutputSchema : public AgentOutputSchemaBase {
public:
    AgentOutputSchema(const std::string& schema_type) : schema_type_(schema_type) {}
    
    std::string get_schema_type() const override { return schema_type_; }
    bool validate(const std::any& output) const override;

private:
    std::string schema_type_;
};

} // namespace openai_agents