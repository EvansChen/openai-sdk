#pragma once

/**
 * Model Context Protocol (MCP) module
 * 
 * This module provides implementations for the Model Context Protocol,
 * enabling communication with MCP servers through various transports
 * including stdio, SSE, and streamable HTTP.
 */

#include "server.h"
#include "util.h"

namespace openai_agents {
namespace mcp {

// Re-export core server classes
using MCPServer = MCPServer;
using MCPServerWithClientSession = MCPServerWithClientSession;
using MCPServerStdio = MCPServerStdio;
using MCPServerSse = MCPServerSse;
using MCPServerStreamableHttp = MCPServerStreamableHttp;

// Re-export parameter structures
using MCPServerStdioParams = MCPServerStdioParams;
using MCPServerSseParams = MCPServerSseParams;
using MCPServerStreamableHttpParams = MCPServerStreamableHttpParams;

// Re-export utility classes and types
using MCPUtil = MCPUtil;
using ToolFilter = ToolFilter;
using ToolFilterCallable = ToolFilterCallable;
using ToolFilterContext = ToolFilterContext;
using ToolFilterStatic = ToolFilterStatic;

// Re-export data structures
using Tool = Tool;
using CallToolResult = CallToolResult;
using GetPromptResult = GetPromptResult;
using ListPromptsResult = ListPromptsResult;
using InitializeResult = InitializeResult;
using ContentItem = ContentItem;
using PromptInfo = PromptInfo;

// Re-export exceptions
using MCPException = MCPException;
using ToolFilterException = ToolFilterException;
using ToolInvocationException = ToolInvocationException;

// Re-export utility functions
using create_static_tool_filter = create_static_tool_filter;

// Convenience factory functions
/**
 * Create a stdio-based MCP server
 */
inline std::shared_ptr<MCPServerStdio> create_stdio_server(
    const MCPServerStdioParams& params,
    bool cache_tools_list = false,
    const std::optional<std::string>& name = std::nullopt,
    std::optional<double> client_session_timeout_seconds = 5.0,
    std::optional<ToolFilter> tool_filter = std::nullopt,
    bool use_structured_content = false
) {
    return std::make_shared<MCPServerStdio>(
        params, cache_tools_list, name, client_session_timeout_seconds,
        tool_filter, use_structured_content
    );
}

/**
 * Create an SSE-based MCP server
 */
inline std::shared_ptr<MCPServerSse> create_sse_server(
    const MCPServerSseParams& params,
    bool cache_tools_list = false,
    const std::optional<std::string>& name = std::nullopt,
    std::optional<double> client_session_timeout_seconds = 5.0,
    std::optional<ToolFilter> tool_filter = std::nullopt,
    bool use_structured_content = false
) {
    return std::make_shared<MCPServerSse>(
        params, cache_tools_list, name, client_session_timeout_seconds,
        tool_filter, use_structured_content
    );
}

/**
 * Create a streamable HTTP-based MCP server
 */
inline std::shared_ptr<MCPServerStreamableHttp> create_streamable_http_server(
    const MCPServerStreamableHttpParams& params,
    bool cache_tools_list = false,
    const std::optional<std::string>& name = std::nullopt,
    std::optional<double> client_session_timeout_seconds = 5.0,
    std::optional<ToolFilter> tool_filter = std::nullopt,
    bool use_structured_content = false
) {
    return std::make_shared<MCPServerStreamableHttp>(
        params, cache_tools_list, name, client_session_timeout_seconds,
        tool_filter, use_structured_content
    );
}

/**
 * Create a simple allowlist tool filter
 */
inline std::optional<ToolFilterStatic> create_allowlist_filter(
    const std::vector<std::string>& allowed_tools
) {
    return create_static_tool_filter(allowed_tools, std::nullopt);
}

/**
 * Create a simple blocklist tool filter
 */
inline std::optional<ToolFilterStatic> create_blocklist_filter(
    const std::vector<std::string>& blocked_tools
) {
    return create_static_tool_filter(std::nullopt, blocked_tools);
}

} // namespace mcp
} // namespace openai_agents