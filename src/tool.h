#pragma once

#include <string>
#include <vector>
#include <memory>
#include <any>

namespace openai_agents {

// Base classes and interfaces for tools
class Tool {
public:
    virtual ~Tool() = default;
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::any execute(const std::any& input) = 0;
};

class FunctionTool : public Tool {
public:
    FunctionTool(const std::string& name, const std::string& description);
    std::string get_name() const override { return name_; }
    std::string get_description() const override { return description_; }
    
private:
    std::string name_;
    std::string description_;
};

struct FunctionToolResult {
    std::any result;
    bool success;
    std::string error_message;
};

// Tool creation function
template<typename Func>
std::shared_ptr<FunctionTool> function_tool(Func func, const std::string& name, const std::string& description);

// Other tool types
class ComputerTool : public Tool {};
class FileSearchTool : public Tool {};
class CodeInterpreterTool : public Tool {};
class ImageGenerationTool : public Tool {};
class WebSearchTool : public Tool {};
class LocalShellTool : public Tool {};
class HostedMCPTool : public Tool {};

} // namespace openai_agents