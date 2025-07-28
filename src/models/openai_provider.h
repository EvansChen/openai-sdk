#pragma once

#include "interface.h"
#include <string>

namespace openai_agents {
namespace models {

class OpenAIProvider : public ModelProvider {
public:
    OpenAIProvider(const std::string& api_key);
    
    std::shared_ptr<Model> get_model(const std::string& model_name) override;
    std::vector<std::string> list_models() const override;

private:
    std::string api_key_;
};

} // namespace models
} // namespace openai_agents