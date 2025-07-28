#pragma once

/**
 * MCP utility functions and tool filter implementations
 */

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <variant>
#include <any>

#include "../run_context.h"

namespace openai_agents {

// Forward declarations
class AgentBase;
class Tool;
class FunctionTool;

namespace mcp {

// Forward declarations
struct Tool;
class MCPServer;

/**
 * Context information available to tool filter functions
 */
struct ToolFilterContext {
    RunContextWrapper run_context;          ///< The current run context
    std::shared_ptr<AgentBase> agent;       ///< The agent requesting the tool list
    std::string server_name;                ///< The name of the MCP server
};

/**
 * Function type for dynamic tool filtering
 * 
 * @param context The context information including run context, agent, and server name
 * @param tool The MCP tool to filter
 * @return Whether the tool should be available (true) or filtered out (false)
 */
using ToolFilterCallable = std::function<bool(const ToolFilterContext&, const Tool&)>;

/**
 * Static tool filter configuration using allowlists and blocklists
 */
struct ToolFilterStatic {
    std::optional<std::vector<std::string>> allowed_tool_names;   ///< Optional allowlist (whitelist)
    std::optional<std::vector<std::string>> blocked_tool_names;   ///< Optional blocklist (blacklist)
};

/**
 * A tool filter that can be either a function, static configuration, or None (no filtering)
 */
using ToolFilter = std::variant<ToolFilterCallable, ToolFilterStatic>;

/**
 * Create a static tool filter from allowlist and blocklist parameters
 * 
 * This is a convenience function for creating a ToolFilterStatic.
 * 
 * @param allowed_tool_names Optional list of tool names to allow (whitelist)
 * @param blocked_tool_names Optional list of tool names to exclude (blacklist)
 * @return A ToolFilterStatic if any filtering is specified, nullopt otherwise
 */
std::optional<ToolFilterStatic> create_static_tool_filter(
    const std::optional<std::vector<std::string>>& allowed_tool_names = std::nullopt,
    const std::optional<std::vector<std::string>>& blocked_tool_names = std::nullopt
);

/**
 * Set of utilities for interop between MCP and Agents SDK tools
 */
class MCPUtil {
public:
    /**
     * Get all function tools from a list of MCP servers
     * 
     * @param servers List of MCP servers to get tools from
     * @param convert_schemas_to_strict Whether to convert schemas to strict format
     * @param run_context The current run context
     * @param agent The agent requesting the tools
     * @return List of function tools from all servers
     * @throws UserError if duplicate tool names are found across servers
     */
    static std::future<std::vector<std::shared_ptr<openai_agents::Tool>>> get_all_function_tools(
        const std::vector<std::shared_ptr<MCPServer>>& servers,
        bool convert_schemas_to_strict,
        const RunContextWrapper& run_context,
        const std::shared_ptr<AgentBase>& agent
    );

    /**
     * Get all function tools from a single MCP server
     * 
     * @param server The MCP server to get tools from
     * @param convert_schemas_to_strict Whether to convert schemas to strict format
     * @param run_context The current run context
     * @param agent The agent requesting the tools
     * @return List of function tools from the server
     */
    static std::future<std::vector<std::shared_ptr<openai_agents::Tool>>> get_function_tools(
        const std::shared_ptr<MCPServer>& server,
        bool convert_schemas_to_strict,
        const RunContextWrapper& run_context,
        const std::shared_ptr<AgentBase>& agent
    );

    /**
     * Convert an MCP tool to an Agents SDK function tool
     * 
     * @param tool The MCP tool to convert
     * @param server The MCP server the tool belongs to
     * @param convert_schemas_to_strict Whether to convert schema to strict format
     * @return Function tool equivalent of the MCP tool
     */
    static std::shared_ptr<FunctionTool> to_function_tool(
        const Tool& tool,
        const std::shared_ptr<MCPServer>& server,
        bool convert_schemas_to_strict
    );

    /**
     * Invoke an MCP tool and return the result as a string
     * 
     * @param server The MCP server to invoke the tool on
     * @param tool The MCP tool to invoke
     * @param context The run context
     * @param input_json The JSON input for the tool
     * @return The result of the tool invocation as a string
     */
    static std::future<std::string> invoke_mcp_tool(
        const std::shared_ptr<MCPServer>& server,
        const Tool& tool,
        const RunContextWrapper& context,
        const std::string& input_json
    );

private:
    /**
     * Parse JSON input safely
     */
    static std::unordered_map<std::string, std::any> parse_json_input(
        const std::string& input_json,
        const std::string& tool_name
    );

    /**
     * Format tool output from MCP result
     */
    static std::string format_tool_output(
        const struct CallToolResult& result,
        const std::shared_ptr<MCPServer>& server
    );
};

/**
 * Exception thrown when MCP operations fail
 */
class MCPException : public std::runtime_error {
public:
    explicit MCPException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * Exception thrown when tool filtering fails
 */
class ToolFilterException : public MCPException {
public:
    explicit ToolFilterException(const std::string& message) : MCPException(message) {}
};

/**
 * Exception thrown when tool invocation fails
 */
class ToolInvocationException : public MCPException {
public:
    explicit ToolInvocationException(const std::string& message) : MCPException(message) {}
};

/**
 * Utility functions for MCP tool management
 */
namespace utils {

/**
 * Validate a tool name for MCP compatibility
 */
bool is_valid_tool_name(const std::string& name);

/**
 * Sanitize a tool name for MCP compatibility
 */
std::string sanitize_tool_name(const std::string& name);

/**
 * Check if two tools have compatible schemas
 */
bool are_tools_compatible(const Tool& tool1, const Tool& tool2);

/**
 * Merge multiple tool schemas (for tool composition)
 */
std::unordered_map<std::string, std::any> merge_tool_schemas(
    const std::vector<std::unordered_map<std::string, std::any>>& schemas
);

/**
 * Extract tool metadata from MCP tool
 */
std::unordered_map<std::string, std::string> extract_tool_metadata(const Tool& tool);

} // namespace utils

} // namespace mcp
} // namespace openai_agents