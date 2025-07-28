#pragma once

#include <string>
#include <memory>

namespace openai_agents {
namespace models {

// Base model interface
class Model {
public:
    virtual ~Model() = default;
    virtual std::string get_name() const = 0;
    virtual std::string generate(const std::string& prompt) = 0;
};

// Model provider interface
class ModelProvider {
public:
    virtual ~ModelProvider() = default;
    virtual std::shared_ptr<Model> get_model(const std::string& model_name) = 0;
    virtual std::vector<std::string> list_models() const = 0;
};

// Model tracing interface
class ModelTracing {
public:
    virtual ~ModelTracing() = default;
    virtual void start_trace(const std::string& trace_id) = 0;
    virtual void end_trace(const std::string& trace_id) = 0;
};

} // namespace models
} // namespace openai_agents