#pragma once

/**
 * Model Context Protocol (MCP) server implementations
 * 
 * This module provides different server implementations for the Model Context Protocol,
 * including stdio, SSE, and streamable HTTP transports.
 */

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <future>
#include <chrono>
#include <mutex>
#include <atomic>
#include <any>

#include "../exceptions.h"
#include "../logger.h"
#include "../run_context.h"
#include "util.h"

namespace openai_agents {

// Forward declarations
class AgentBase;

namespace mcp {

// Forward declarations
struct Tool;
struct CallToolResult;
struct GetPromptResult;
struct ListPromptsResult;
struct InitializeResult;

/**
 * Base abstract class for Model Context Protocol servers
 */
class MCPServer {
public:
    explicit MCPServer(bool use_structured_content = false);
    virtual ~MCPServer() = default;

    /**
     * Connect to the server. For example, this might mean spawning a subprocess or
     * opening a network connection. The server is expected to remain connected until
     * cleanup() is called.
     */
    virtual std::future<void> connect() = 0;

    /**
     * A readable name for the server
     */
    virtual std::string name() const = 0;

    /**
     * Cleanup the server. For example, this might mean closing a subprocess or
     * closing a network connection.
     */
    virtual std::future<void> cleanup() = 0;

    /**
     * List the tools available on the server
     */
    virtual std::future<std::vector<Tool>> list_tools(
        const std::optional<RunContextWrapper>& run_context = std::nullopt,
        const std::shared_ptr<AgentBase>& agent = nullptr
    ) = 0;

    /**
     * Invoke a tool on the server
     */
    virtual std::future<CallToolResult> call_tool(
        const std::string& tool_name,
        const std::optional<std::unordered_map<std::string, std::any>>& arguments = std::nullopt
    ) = 0;

    /**
     * List the prompts available on the server
     */
    virtual std::future<ListPromptsResult> list_prompts() = 0;

    /**
     * Get a specific prompt from the server
     */
    virtual std::future<GetPromptResult> get_prompt(
        const std::string& name,
        const std::optional<std::unordered_map<std::string, std::any>>& arguments = std::nullopt
    ) = 0;

    bool use_structured_content;

protected:
    std::mutex cleanup_mutex_;
};

/**
 * Message interface for MCP communication
 */
struct SessionMessage {
    std::string type;
    std::string id;
    std::unordered_map<std::string, std::any> data;
    
    virtual ~SessionMessage() = default;
};

/**
 * Stream interface for MCP communication
 */
template<typename T>
class Stream {
public:
    virtual ~Stream() = default;
    virtual std::future<std::optional<T>> read() = 0;
    virtual std::future<void> write(const T& message) = 0;
    virtual std::future<void> close() = 0;
};

using ReceiveStream = Stream<SessionMessage>;
using SendStream = Stream<SessionMessage>;

/**
 * Client session interface for MCP communication
 */
class ClientSession {
public:
    ClientSession(
        std::shared_ptr<ReceiveStream> read_stream,
        std::shared_ptr<SendStream> write_stream,
        std::optional<std::chrono::milliseconds> timeout = std::nullopt
    );
    
    virtual ~ClientSession() = default;

    std::future<InitializeResult> initialize();
    std::future<std::vector<Tool>> list_tools();
    std::future<CallToolResult> call_tool(
        const std::string& tool_name,
        const std::optional<std::unordered_map<std::string, std::any>>& arguments = std::nullopt
    );
    std::future<ListPromptsResult> list_prompts();
    std::future<GetPromptResult> get_prompt(
        const std::string& name,
        const std::optional<std::unordered_map<std::string, std::any>>& arguments = std::nullopt
    );

private:
    std::shared_ptr<ReceiveStream> read_stream_;
    std::shared_ptr<SendStream> write_stream_;
    std::optional<std::chrono::milliseconds> timeout_;
    std::atomic<bool> initialized_{false};
};

/**
 * Base class for MCP servers that use a ClientSession to communicate
 */
class MCPServerWithClientSession : public MCPServer {
public:
    MCPServerWithClientSession(
        bool cache_tools_list,
        std::optional<double> client_session_timeout_seconds,
        std::optional<ToolFilter> tool_filter = std::nullopt,
        bool use_structured_content = false
    );

    virtual ~MCPServerWithClientSession();

    // MCPServer interface implementation
    std::future<void> connect() override;
    std::future<std::vector<Tool>> list_tools(
        const std::optional<RunContextWrapper>& run_context = std::nullopt,
        const std::shared_ptr<AgentBase>& agent = nullptr
    ) override;
    std::future<CallToolResult> call_tool(
        const std::string& tool_name,
        const std::optional<std::unordered_map<std::string, std::any>>& arguments = std::nullopt
    ) override;
    std::future<ListPromptsResult> list_prompts() override;
    std::future<GetPromptResult> get_prompt(
        const std::string& name,
        const std::optional<std::unordered_map<std::string, std::any>>& arguments = std::nullopt
    ) override;
    std::future<void> cleanup() override;

    /**
     * Invalidate the tools cache
     */
    void invalidate_tools_cache();

protected:
    /**
     * Create the streams for the server - must be implemented by subclasses
     */
    virtual std::future<std::tuple<
        std::shared_ptr<ReceiveStream>,
        std::shared_ptr<SendStream>
    >> create_streams() = 0;

private:
    std::future<std::vector<Tool>> apply_tool_filter(
        const std::vector<Tool>& tools,
        const RunContextWrapper& run_context,
        const std::shared_ptr<AgentBase>& agent
    );

    std::vector<Tool> apply_static_tool_filter(
        const std::vector<Tool>& tools,
        const ToolFilterStatic& static_filter
    );

    std::future<std::vector<Tool>> apply_dynamic_tool_filter(
        const std::vector<Tool>& tools,
        const RunContextWrapper& run_context,
        const std::shared_ptr<AgentBase>& agent
    );

    std::shared_ptr<ClientSession> session_;
    bool cache_tools_list_;
    std::optional<double> client_session_timeout_seconds_;
    std::optional<ToolFilter> tool_filter_;
    
    std::atomic<bool> cache_dirty_{true};
    std::optional<std::vector<Tool>> cached_tools_;
    std::mutex tools_cache_mutex_;
    
    std::optional<InitializeResult> server_initialize_result_;
};

/**
 * Parameters for stdio-based MCP server
 */
struct MCPServerStdioParams {
    std::string command;                                           ///< The executable to run
    std::vector<std::string> args;                                ///< Command line arguments  
    std::unordered_map<std::string, std::string> env;            ///< Environment variables
    std::optional<std::string> cwd;                               ///< Working directory
    std::string encoding = "utf-8";                              ///< Text encoding
    std::string encoding_error_handler = "strict";               ///< Encoding error handler
};

/**
 * MCP server implementation using stdio transport
 */
class MCPServerStdio : public MCPServerWithClientSession {
public:
    MCPServerStdio(
        const MCPServerStdioParams& params,
        bool cache_tools_list = false,
        const std::optional<std::string>& name = std::nullopt,
        std::optional<double> client_session_timeout_seconds = 5.0,
        std::optional<ToolFilter> tool_filter = std::nullopt,
        bool use_structured_content = false
    );

    std::string name() const override;

protected:
    std::future<std::tuple<
        std::shared_ptr<ReceiveStream>,
        std::shared_ptr<SendStream>
    >> create_streams() override;

private:
    MCPServerStdioParams params_;
    std::string name_;
};

/**
 * Parameters for SSE-based MCP server
 */
struct MCPServerSseParams {
    std::string url;                                              ///< The URL of the server
    std::unordered_map<std::string, std::string> headers;        ///< Headers to send
    double timeout = 5.0;                                        ///< HTTP request timeout
    double sse_read_timeout = 300.0;                             ///< SSE connection timeout (5 minutes)
};

/**
 * MCP server implementation using HTTP with SSE transport
 */
class MCPServerSse : public MCPServerWithClientSession {
public:
    MCPServerSse(
        const MCPServerSseParams& params,
        bool cache_tools_list = false,
        const std::optional<std::string>& name = std::nullopt,
        std::optional<double> client_session_timeout_seconds = 5.0,
        std::optional<ToolFilter> tool_filter = std::nullopt,
        bool use_structured_content = false
    );

    std::string name() const override;

protected:
    std::future<std::tuple<
        std::shared_ptr<ReceiveStream>,
        std::shared_ptr<SendStream>
    >> create_streams() override;

private:
    MCPServerSseParams params_;
    std::string name_;
};

/**
 * Parameters for streamable HTTP-based MCP server
 */
struct MCPServerStreamableHttpParams {
    std::string url;                                              ///< The URL of the server
    std::unordered_map<std::string, std::string> headers;        ///< Headers to send
    double timeout = 5.0;                                        ///< HTTP request timeout
    double sse_read_timeout = 300.0;                             ///< Connection timeout (5 minutes)
    bool terminate_on_close = true;                              ///< Terminate on close
};

/**
 * MCP server implementation using streamable HTTP transport
 */
class MCPServerStreamableHttp : public MCPServerWithClientSession {
public:
    MCPServerStreamableHttp(
        const MCPServerStreamableHttpParams& params,
        bool cache_tools_list = false,
        const std::optional<std::string>& name = std::nullopt,
        std::optional<double> client_session_timeout_seconds = 5.0,
        std::optional<ToolFilter> tool_filter = std::nullopt,
        bool use_structured_content = false
    );

    std::string name() const override;

protected:
    std::future<std::tuple<
        std::shared_ptr<ReceiveStream>,
        std::shared_ptr<SendStream>
    >> create_streams() override;

private:
    MCPServerStreamableHttpParams params_;
    std::string name_;
};

// MCP data structures
struct Tool {
    std::string name;
    std::string description;
    std::unordered_map<std::string, std::any> input_schema;
};

struct ContentItem {
    std::string type;
    std::unordered_map<std::string, std::any> data;
    
    std::string to_json() const;
    static ContentItem from_json(const std::string& json);
};

struct CallToolResult {
    std::vector<ContentItem> content;
    std::optional<std::unordered_map<std::string, std::any>> structured_content;
    bool is_error = false;
};

struct PromptInfo {
    std::string name;
    std::string description;
    std::vector<std::string> arguments;
};

struct ListPromptsResult {
    std::vector<PromptInfo> prompts;
};

struct GetPromptResult {
    std::vector<ContentItem> messages;
    std::string description;
};

struct InitializeResult {
    std::string protocol_version;
    std::unordered_map<std::string, std::any> capabilities;
    std::unordered_map<std::string, std::any> server_info;
};

} // namespace mcp
} // namespace openai_agents