#pragma once

#include "interface.h"
#include <string>

namespace openai_agents {
namespace models {

class OpenAIChatCompletionsModel : public Model {
public:
    OpenAIChatCompletionsModel(const std::string& model_name, const std::string& api_key);
    
    std::string get_name() const override;
    std::string generate(const std::string& prompt) override;

private:
    std::string model_name_;
    std::string api_key_;
};

} // namespace models
} // namespace openai_agents