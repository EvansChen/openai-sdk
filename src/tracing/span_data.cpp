#include "span_data.h"
#include <sstream>

namespace openai_agents {
namespace tracing {

// AgentSpanData implementation
AgentSpanData::AgentSpanData(
    const std::string& name,
    const std::optional<std::vector<std::string>>& handoffs,
    const std::optional<std::vector<std::string>>& tools,
    const std::optional<std::string>& output_type
) : name(name), handoffs(handoffs), tools(tools), output_type(output_type) {}

nlohmann::json AgentSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    result["name"] = name;
    
    if (handoffs.has_value()) {
        result["handoffs"] = handoffs.value();
    } else {
        result["handoffs"] = nullptr;
    }
    
    if (tools.has_value()) {
        result["tools"] = tools.value();
    } else {
        result["tools"] = nullptr;
    }
    
    if (output_type.has_value()) {
        result["output_type"] = output_type.value();
    } else {
        result["output_type"] = nullptr;
    }
    
    return result;
}

std::unique_ptr<SpanData> AgentSpanData::clone() const {
    return std::make_unique<AgentSpanData>(name, handoffs, tools, output_type);
}

// FunctionSpanData implementation
FunctionSpanData::FunctionSpanData(
    const std::string& name,
    const std::optional<std::string>& input,
    const std::any& output,
    const std::optional<nlohmann::json>& mcp_data
) : name(name), input(input), output(output), mcp_data(mcp_data) {}

nlohmann::json FunctionSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    result["name"] = name;
    
    if (input.has_value()) {
        result["input"] = input.value();
    } else {
        result["input"] = nullptr;
    }
    
    // Convert std::any to string representation
    if (output.has_value()) {
        try {
            // Try different types
            if (output.type() == typeid(std::string)) {
                result["output"] = std::any_cast<std::string>(output);
            } else if (output.type() == typeid(int)) {
                result["output"] = std::to_string(std::any_cast<int>(output));
            } else if (output.type() == typeid(double)) {
                result["output"] = std::to_string(std::any_cast<double>(output));
            } else if (output.type() == typeid(bool)) {
                result["output"] = std::any_cast<bool>(output) ? "true" : "false";
            } else {
                result["output"] = std::string("<") + output.type().name() + " object>";
            }
        } catch (const std::bad_any_cast&) {
            result["output"] = std::string("<") + output.type().name() + " object>";
        }
    } else {
        result["output"] = nullptr;
    }
    
    if (mcp_data.has_value()) {
        result["mcp_data"] = mcp_data.value();
    } else {
        result["mcp_data"] = nullptr;
    }
    
    return result;
}

std::unique_ptr<SpanData> FunctionSpanData::clone() const {
    return std::make_unique<FunctionSpanData>(name, input, output, mcp_data);
}

// GenerationSpanData implementation
GenerationSpanData::GenerationSpanData(
    const std::optional<std::vector<nlohmann::json>>& input,
    const std::optional<std::vector<nlohmann::json>>& output,
    const std::optional<std::string>& model,
    const std::optional<nlohmann::json>& model_config,
    const std::optional<nlohmann::json>& usage
) : input(input), output(output), model(model), model_config(model_config), usage(usage) {}

nlohmann::json GenerationSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    
    if (input.has_value()) {
        result["input"] = input.value();
    } else {
        result["input"] = nullptr;
    }
    
    if (output.has_value()) {
        result["output"] = output.value();
    } else {
        result["output"] = nullptr;
    }
    
    if (model.has_value()) {
        result["model"] = model.value();
    } else {
        result["model"] = nullptr;
    }
    
    if (model_config.has_value()) {
        result["model_config"] = model_config.value();
    } else {
        result["model_config"] = nullptr;
    }
    
    if (usage.has_value()) {
        result["usage"] = usage.value();
    } else {
        result["usage"] = nullptr;
    }
    
    return result;
}

std::unique_ptr<SpanData> GenerationSpanData::clone() const {
    return std::make_unique<GenerationSpanData>(input, output, model, model_config, usage);
}

// ResponseSpanData implementation
ResponseSpanData::ResponseSpanData(
    const std::optional<std::string>& response_id,
    const std::optional<std::any>& input
) : response_id(response_id), input(input) {}

nlohmann::json ResponseSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    
    if (response_id.has_value()) {
        result["response_id"] = response_id.value();
    } else {
        result["response_id"] = nullptr;
    }
    
    return result;
}

std::unique_ptr<SpanData> ResponseSpanData::clone() const {
    return std::make_unique<ResponseSpanData>(response_id, input);
}

// HandoffSpanData implementation
HandoffSpanData::HandoffSpanData(
    const std::optional<std::string>& from_agent,
    const std::optional<std::string>& to_agent
) : from_agent(from_agent), to_agent(to_agent) {}

nlohmann::json HandoffSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    
    if (from_agent.has_value()) {
        result["from_agent"] = from_agent.value();
    } else {
        result["from_agent"] = nullptr;
    }
    
    if (to_agent.has_value()) {
        result["to_agent"] = to_agent.value();
    } else {
        result["to_agent"] = nullptr;
    }
    
    return result;
}

std::unique_ptr<SpanData> HandoffSpanData::clone() const {
    return std::make_unique<HandoffSpanData>(from_agent, to_agent);
}

// CustomSpanData implementation
CustomSpanData::CustomSpanData(
    const std::string& name,
    const nlohmann::json& data
) : name(name), data(data) {}

nlohmann::json CustomSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    result["name"] = name;
    result["data"] = data;
    return result;
}

std::unique_ptr<SpanData> CustomSpanData::clone() const {
    return std::make_unique<CustomSpanData>(name, data);
}

// GuardrailSpanData implementation
GuardrailSpanData::GuardrailSpanData(
    const std::string& name,
    bool triggered
) : name(name), triggered(triggered) {}

nlohmann::json GuardrailSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    result["name"] = name;
    result["triggered"] = triggered;
    return result;
}

std::unique_ptr<SpanData> GuardrailSpanData::clone() const {
    return std::make_unique<GuardrailSpanData>(name, triggered);
}

// TranscriptionSpanData implementation
TranscriptionSpanData::TranscriptionSpanData(
    const std::optional<std::string>& input,
    const std::string& input_format,
    const std::optional<std::string>& output,
    const std::optional<std::string>& model,
    const std::optional<nlohmann::json>& model_config
) : input(input), input_format(input_format), output(output), model(model), model_config(model_config) {}

nlohmann::json TranscriptionSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    
    nlohmann::json input_obj;
    input_obj["data"] = input.value_or("");
    input_obj["format"] = input_format;
    result["input"] = input_obj;
    
    if (output.has_value()) {
        result["output"] = output.value();
    } else {
        result["output"] = nullptr;
    }
    
    if (model.has_value()) {
        result["model"] = model.value();
    } else {
        result["model"] = nullptr;
    }
    
    if (model_config.has_value()) {
        result["model_config"] = model_config.value();
    } else {
        result["model_config"] = nullptr;
    }
    
    return result;
}

std::unique_ptr<SpanData> TranscriptionSpanData::clone() const {
    return std::make_unique<TranscriptionSpanData>(input, input_format, output, model, model_config);
}

// SpeechSpanData implementation
SpeechSpanData::SpeechSpanData(
    const std::optional<std::string>& input,
    const std::optional<std::string>& output,
    const std::string& output_format,
    const std::optional<std::string>& model,
    const std::optional<nlohmann::json>& model_config,
    const std::optional<std::string>& first_content_at
) : input(input), output(output), output_format(output_format), 
    model(model), model_config(model_config), first_content_at(first_content_at) {}

nlohmann::json SpeechSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    
    if (input.has_value()) {
        result["input"] = input.value();
    } else {
        result["input"] = nullptr;
    }
    
    nlohmann::json output_obj;
    output_obj["data"] = output.value_or("");
    output_obj["format"] = output_format;
    result["output"] = output_obj;
    
    if (model.has_value()) {
        result["model"] = model.value();
    } else {
        result["model"] = nullptr;
    }
    
    if (model_config.has_value()) {
        result["model_config"] = model_config.value();
    } else {
        result["model_config"] = nullptr;
    }
    
    if (first_content_at.has_value()) {
        result["first_content_at"] = first_content_at.value();
    } else {
        result["first_content_at"] = nullptr;
    }
    
    return result;
}

std::unique_ptr<SpanData> SpeechSpanData::clone() const {
    return std::make_unique<SpeechSpanData>(input, output, output_format, model, model_config, first_content_at);
}

// SpeechGroupSpanData implementation
SpeechGroupSpanData::SpeechGroupSpanData(
    const std::optional<std::string>& input
) : input(input) {}

nlohmann::json SpeechGroupSpanData::export_data() const {
    nlohmann::json result;
    result["type"] = get_type();
    
    if (input.has_value()) {
        result["input"] = input.value();
    } else {
        result["input"] = nullptr;
    }
    
    return result;
}

std::unique_ptr<SpanData> SpeechGroupSpanData::clone() const {
    return std::make_unique<SpeechGroupSpanData>(input);
}

// MCPListToolsSpanData implementation
MCPListToolsSpanData::MCPListToolsSpanData(
    const std::optional<std::string>& server,
    const std::optional<std::vector<std::string>>& result
) : server(server), result(result) {}

nlohmann::json MCPListToolsSpanData::export_data() const {
    nlohmann::json json_result;
    json_result["type"] = get_type();
    
    if (server.has_value()) {
        json_result["server"] = server.value();
    } else {
        json_result["server"] = nullptr;
    }
    
    if (result.has_value()) {
        json_result["result"] = result.value();
    } else {
        json_result["result"] = nullptr;
    }
    
    return json_result;
}

std::unique_ptr<SpanData> MCPListToolsSpanData::clone() const {
    return std::make_unique<MCPListToolsSpanData>(server, result);
}

} // namespace tracing
} // namespace openai_agents