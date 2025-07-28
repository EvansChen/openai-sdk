#include "server.h"
#include "../agent.h"
#include "../util/_json.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <stdexcept>

namespace openai_agents {
namespace mcp {

// MCPServer implementation
MCPServer::MCPServer(bool use_structured_content)
    : use_structured_content(use_structured_content) {
}

// ClientSession implementation
ClientSession::ClientSession(
    std::shared_ptr<ReceiveStream> read_stream,
    std::shared_ptr<SendStream> write_stream,
    std::optional<std::chrono::milliseconds> timeout
) : read_stream_(std::move(read_stream)),
    write_stream_(std::move(write_stream)),
    timeout_(timeout) {
}

std::future<InitializeResult> ClientSession::initialize() {
    return std::async(std::launch::async, [this]() -> InitializeResult {
        if (initialized_.exchange(true)) {
            throw AgentsException("ClientSession already initialized");
        }

        // Send initialize message
        SessionMessage init_msg;
        init_msg.type = "initialize";
        init_msg.id = "init_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        write_stream_->write(init_msg).wait();

        // Wait for response
        auto response = read_stream_->read().get();
        if (!response) {
            throw AgentsException("No response received for initialize");
        }

        InitializeResult result;
        result.protocol_version = "2024-11-05";
        result.capabilities = {{"tools", true}, {"prompts", true}};
        result.server_info = {{"name", "MCP Server"}, {"version", "1.0.0"}};

        return result;
    });
}

std::future<std::vector<Tool>> ClientSession::list_tools() {
    return std::async(std::launch::async, [this]() -> std::vector<Tool> {
        if (!initialized_.load()) {
            throw AgentsException("ClientSession not initialized");
        }

        SessionMessage msg;
        msg.type = "tools/list";
        msg.id = "list_tools_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

        write_stream_->write(msg).wait();

        auto response = read_stream_->read().get();
        if (!response) {
            throw AgentsException("No response received for list_tools");
        }

        // Parse tools from response
        std::vector<Tool> tools;
        // Implementation would parse actual MCP response
        return tools;
    });
}

std::future<CallToolResult> ClientSession::call_tool(
    const std::string& tool_name,
    const std::optional<std::unordered_map<std::string, std::any>>& arguments
) {
    return std::async(std::launch::async, [this, tool_name, arguments]() -> CallToolResult {
        if (!initialized_.load()) {
            throw AgentsException("ClientSession not initialized");
        }

        SessionMessage msg;
        msg.type = "tools/call";
        msg.id = "call_tool_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        msg.data["name"] = tool_name;
        if (arguments) {
            msg.data["arguments"] = *arguments;
        }

        write_stream_->write(msg).wait();

        auto response = read_stream_->read().get();
        if (!response) {
            throw AgentsException("No response received for call_tool");
        }

        CallToolResult result;
        result.content = {};  // Would parse from actual response
        return result;
    });
}

std::future<ListPromptsResult> ClientSession::list_prompts() {
    return std::async(std::launch::async, [this]() -> ListPromptsResult {
        if (!initialized_.load()) {
            throw AgentsException("ClientSession not initialized");
        }

        SessionMessage msg;
        msg.type = "prompts/list";
        msg.id = "list_prompts_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

        write_stream_->write(msg).wait();

        auto response = read_stream_->read().get();
        if (!response) {
            throw AgentsException("No response received for list_prompts");
        }

        ListPromptsResult result;
        result.prompts = {};  // Would parse from actual response
        return result;
    });
}

std::future<GetPromptResult> ClientSession::get_prompt(
    const std::string& name,
    const std::optional<std::unordered_map<std::string, std::any>>& arguments
) {
    return std::async(std::launch::async, [this, name, arguments]() -> GetPromptResult {
        if (!initialized_.load()) {
            throw AgentsException("ClientSession not initialized");
        }

        SessionMessage msg;
        msg.type = "prompts/get";
        msg.id = "get_prompt_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        msg.data["name"] = name;
        if (arguments) {
            msg.data["arguments"] = *arguments;
        }

        write_stream_->write(msg).wait();

        auto response = read_stream_->read().get();
        if (!response) {
            throw AgentsException("No response received for get_prompt");
        }

        GetPromptResult result;
        result.messages = {};  // Would parse from actual response
        result.description = "";
        return result;
    });
}

// MCPServerWithClientSession implementation
MCPServerWithClientSession::MCPServerWithClientSession(
    bool cache_tools_list,
    std::optional<double> client_session_timeout_seconds,
    std::optional<ToolFilter> tool_filter,
    bool use_structured_content
) : MCPServer(use_structured_content),
    cache_tools_list_(cache_tools_list),
    client_session_timeout_seconds_(client_session_timeout_seconds),
    tool_filter_(tool_filter) {
}

MCPServerWithClientSession::~MCPServerWithClientSession() {
    try {
        cleanup().wait();
    } catch (const std::exception& e) {
        logger::error("Error during MCPServerWithClientSession destruction: " + std::string(e.what()));
    }
}

std::future<void> MCPServerWithClientSession::connect() {
    return std::async(std::launch::async, [this]() -> void {
        try {
            auto [read_stream, write_stream] = create_streams().get();
            
            std::optional<std::chrono::milliseconds> timeout;
            if (client_session_timeout_seconds_) {
                timeout = std::chrono::milliseconds(
                    static_cast<int>(*client_session_timeout_seconds_ * 1000)
                );
            }

            session_ = std::make_shared<ClientSession>(read_stream, write_stream, timeout);
            server_initialize_result_ = session_->initialize().get();
            
        } catch (const std::exception& e) {
            logger::error("Error initializing MCP server: " + std::string(e.what()));
            cleanup().wait();
            throw;
        }
    });
}

void MCPServerWithClientSession::invalidate_tools_cache() {
    cache_dirty_.store(true);
}

std::future<std::vector<Tool>> MCPServerWithClientSession::list_tools(
    const std::optional<RunContextWrapper>& run_context,
    const std::shared_ptr<AgentBase>& agent
) {
    return std::async(std::launch::async, [this, run_context, agent]() -> std::vector<Tool> {
        if (!session_) {
            throw UserError("Server not initialized. Make sure you call connect() first.");
        }

        std::vector<Tool> tools;

        // Check cache
        {
            std::lock_guard<std::mutex> lock(tools_cache_mutex_);
            if (cache_tools_list_ && !cache_dirty_.load() && cached_tools_) {
                tools = *cached_tools_;
            } else {
                // Reset cache dirty flag and fetch from server
                cache_dirty_.store(false);
                tools = session_->list_tools().get();
                cached_tools_ = tools;
            }
        }

        // Apply tool filter if present
        if (tool_filter_) {
            if (!run_context || !agent) {
                throw UserError("run_context and agent are required for dynamic tool filtering");
            }
            tools = apply_tool_filter(tools, *run_context, agent).get();
        }

        return tools;
    });
}

std::future<CallToolResult> MCPServerWithClientSession::call_tool(
    const std::string& tool_name,
    const std::optional<std::unordered_map<std::string, std::any>>& arguments
) {
    return std::async(std::launch::async, [this, tool_name, arguments]() -> CallToolResult {
        if (!session_) {
            throw UserError("Server not initialized. Make sure you call connect() first.");
        }

        return session_->call_tool(tool_name, arguments).get();
    });
}

std::future<ListPromptsResult> MCPServerWithClientSession::list_prompts() {
    return std::async(std::launch::async, [this]() -> ListPromptsResult {
        if (!session_) {
            throw UserError("Server not initialized. Make sure you call connect() first.");
        }

        return session_->list_prompts().get();
    });
}

std::future<GetPromptResult> MCPServerWithClientSession::get_prompt(
    const std::string& name,
    const std::optional<std::unordered_map<std::string, std::any>>& arguments
) {
    return std::async(std::launch::async, [this, name, arguments]() -> GetPromptResult {
        if (!session_) {
            throw UserError("Server not initialized. Make sure you call connect() first.");
        }

        return session_->get_prompt(name, arguments).get();
    });
}

std::future<void> MCPServerWithClientSession::cleanup() {
    return std::async(std::launch::async, [this]() -> void {
        std::lock_guard<std::mutex> lock(cleanup_mutex_);
        try {
            session_.reset();
        } catch (const std::exception& e) {
            logger::error("Error cleaning up server: " + std::string(e.what()));
        }
    });
}

std::future<std::vector<Tool>> MCPServerWithClientSession::apply_tool_filter(
    const std::vector<Tool>& tools,
    const RunContextWrapper& run_context,
    const std::shared_ptr<AgentBase>& agent
) {
    return std::async(std::launch::async, [this, tools, &run_context, agent]() -> std::vector<Tool> {
        if (!tool_filter_) {
            return tools;
        }

        return std::visit([this, &tools, &run_context, agent](const auto& filter) -> std::vector<Tool> {
            using FilterType = std::decay_t<decltype(filter)>;
            
            if constexpr (std::is_same_v<FilterType, ToolFilterStatic>) {
                return apply_static_tool_filter(tools, filter);
            } else if constexpr (std::is_same_v<FilterType, ToolFilterCallable>) {
                return apply_dynamic_tool_filter(tools, run_context, agent).get();
            } else {
                return tools;
            }
        }, *tool_filter_);
    });
}

std::vector<Tool> MCPServerWithClientSession::apply_static_tool_filter(
    const std::vector<Tool>& tools,
    const ToolFilterStatic& static_filter
) {
    std::vector<Tool> filtered_tools = tools;

    // Apply allowed_tool_names filter (whitelist)
    if (static_filter.allowed_tool_names) {
        const auto& allowed_names = *static_filter.allowed_tool_names;
        filtered_tools.erase(
            std::remove_if(filtered_tools.begin(), filtered_tools.end(),
                [&allowed_names](const Tool& tool) {
                    return std::find(allowed_names.begin(), allowed_names.end(), tool.name) == allowed_names.end();
                }),
            filtered_tools.end()
        );
    }

    // Apply blocked_tool_names filter (blacklist)
    if (static_filter.blocked_tool_names) {
        const auto& blocked_names = *static_filter.blocked_tool_names;
        filtered_tools.erase(
            std::remove_if(filtered_tools.begin(), filtered_tools.end(),
                [&blocked_names](const Tool& tool) {
                    return std::find(blocked_names.begin(), blocked_names.end(), tool.name) != blocked_names.end();
                }),
            filtered_tools.end()
        );
    }

    return filtered_tools;
}

std::future<std::vector<Tool>> MCPServerWithClientSession::apply_dynamic_tool_filter(
    const std::vector<Tool>& tools,
    const RunContextWrapper& run_context,
    const std::shared_ptr<AgentBase>& agent
) {
    return std::async(std::launch::async, [this, tools, &run_context, agent]() -> std::vector<Tool> {
        if (!std::holds_alternative<ToolFilterCallable>(*tool_filter_)) {
            throw std::runtime_error("Tool filter must be callable for dynamic filtering");
        }

        const auto& tool_filter_func = std::get<ToolFilterCallable>(*tool_filter_);

        ToolFilterContext filter_context{
            .run_context = run_context,
            .agent = agent,
            .server_name = name()
        };

        std::vector<Tool> filtered_tools;
        for (const auto& tool : tools) {
            try {
                bool should_include = tool_filter_func(filter_context, tool);
                if (should_include) {
                    filtered_tools.push_back(tool);
                }
            } catch (const std::exception& e) {
                logger::error("Error applying tool filter to tool '" + tool.name + 
                            "' on server '" + name() + "': " + std::string(e.what()));
                // On error, exclude the tool for safety
            }
        }

        return filtered_tools;
    });
}

// MCPServerStdio implementation
MCPServerStdio::MCPServerStdio(
    const MCPServerStdioParams& params,
    bool cache_tools_list,
    const std::optional<std::string>& name,
    std::optional<double> client_session_timeout_seconds,
    std::optional<ToolFilter> tool_filter,
    bool use_structured_content
) : MCPServerWithClientSession(cache_tools_list, client_session_timeout_seconds, tool_filter, use_structured_content),
    params_(params),
    name_(name.value_or("stdio: " + params.command)) {
}

std::string MCPServerStdio::name() const {
    return name_;
}

std::future<std::tuple<std::shared_ptr<ReceiveStream>, std::shared_ptr<SendStream>>> 
MCPServerStdio::create_streams() {
    return std::async(std::launch::async, [this]() -> std::tuple<std::shared_ptr<ReceiveStream>, std::shared_ptr<SendStream>> {
        // Implementation would create actual stdio streams
        // For now, return placeholders
        auto read_stream = std::shared_ptr<ReceiveStream>{};
        auto write_stream = std::shared_ptr<SendStream>{};
        return std::make_tuple(read_stream, write_stream);
    });
}

// MCPServerSse implementation
MCPServerSse::MCPServerSse(
    const MCPServerSseParams& params,
    bool cache_tools_list,
    const std::optional<std::string>& name,
    std::optional<double> client_session_timeout_seconds,
    std::optional<ToolFilter> tool_filter,
    bool use_structured_content
) : MCPServerWithClientSession(cache_tools_list, client_session_timeout_seconds, tool_filter, use_structured_content),
    params_(params),
    name_(name.value_or("sse: " + params.url)) {
}

std::string MCPServerSse::name() const {
    return name_;
}

std::future<std::tuple<std::shared_ptr<ReceiveStream>, std::shared_ptr<SendStream>>> 
MCPServerSse::create_streams() {
    return std::async(std::launch::async, [this]() -> std::tuple<std::shared_ptr<ReceiveStream>, std::shared_ptr<SendStream>> {
        // Implementation would create actual SSE streams
        // For now, return placeholders
        auto read_stream = std::shared_ptr<ReceiveStream>{};
        auto write_stream = std::shared_ptr<SendStream>{};
        return std::make_tuple(read_stream, write_stream);
    });
}

// MCPServerStreamableHttp implementation
MCPServerStreamableHttp::MCPServerStreamableHttp(
    const MCPServerStreamableHttpParams& params,
    bool cache_tools_list,
    const std::optional<std::string>& name,
    std::optional<double> client_session_timeout_seconds,
    std::optional<ToolFilter> tool_filter,
    bool use_structured_content
) : MCPServerWithClientSession(cache_tools_list, client_session_timeout_seconds, tool_filter, use_structured_content),
    params_(params),
    name_(name.value_or("streamable_http: " + params.url)) {
}

std::string MCPServerStreamableHttp::name() const {
    return name_;
}

std::future<std::tuple<std::shared_ptr<ReceiveStream>, std::shared_ptr<SendStream>>> 
MCPServerStreamableHttp::create_streams() {
    return std::async(std::launch::async, [this]() -> std::tuple<std::shared_ptr<ReceiveStream>, std::shared_ptr<SendStream>> {
        // Implementation would create actual streamable HTTP streams
        // For now, return placeholders
        auto read_stream = std::shared_ptr<ReceiveStream>{};
        auto write_stream = std::shared_ptr<SendStream>{};
        return std::make_tuple(read_stream, write_stream);
    });
}

// ContentItem implementation
std::string ContentItem::to_json() const {
    // Implementation would serialize to JSON
    return "{}";
}

ContentItem ContentItem::from_json(const std::string& json) {
    // Implementation would parse from JSON
    ContentItem item;
    item.type = "text";
    return item;
}

} // namespace mcp
} // namespace openai_agents