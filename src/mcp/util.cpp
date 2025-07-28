#include "util.h"
#include "server.h"
#include "../agent.h"
#include "../tool.h"
#include "../function_schema.h"
#include "../strict_schema.h"
#include "../logger.h"
#include "../util/_json.h"
#include "../tracing/spans.h"
#include "../exceptions.h"
#include <algorithm>
#include <regex>
#include <sstream>

namespace openai_agents {
namespace mcp {

// Utility function implementations
std::optional<ToolFilterStatic> create_static_tool_filter(
    const std::optional<std::vector<std::string>>& allowed_tool_names,
    const std::optional<std::vector<std::string>>& blocked_tool_names
) {
    if (!allowed_tool_names && !blocked_tool_names) {
        return std::nullopt;
    }

    ToolFilterStatic filter_dict;
    if (allowed_tool_names) {
        filter_dict.allowed_tool_names = *allowed_tool_names;
    }
    if (blocked_tool_names) {
        filter_dict.blocked_tool_names = *blocked_tool_names;
    }

    return filter_dict;
}

// MCPUtil implementations
std::future<std::vector<std::shared_ptr<openai_agents::Tool>>> MCPUtil::get_all_function_tools(
    const std::vector<std::shared_ptr<MCPServer>>& servers,
    bool convert_schemas_to_strict,
    const RunContextWrapper& run_context,
    const std::shared_ptr<AgentBase>& agent
) {
    return std::async(std::launch::async, [servers, convert_schemas_to_strict, run_context, agent]() 
        -> std::vector<std::shared_ptr<openai_agents::Tool>> {
        
        std::vector<std::shared_ptr<openai_agents::Tool>> all_tools;
        std::unordered_set<std::string> tool_names;
        
        for (const auto& server : servers) {
            auto server_tools = get_function_tools(server, convert_schemas_to_strict, run_context, agent).get();
            
            // Check for duplicate tool names
            std::unordered_set<std::string> server_tool_names;
            for (const auto& tool : server_tools) {
                server_tool_names.insert(tool->get_name());
            }
            
            // Find intersections
            std::vector<std::string> duplicates;
            std::set_intersection(
                tool_names.begin(), tool_names.end(),
                server_tool_names.begin(), server_tool_names.end(),
                std::back_inserter(duplicates)
            );
            
            if (!duplicates.empty()) {
                std::ostringstream oss;
                oss << "Duplicate tool names found across MCP servers: ";
                for (size_t i = 0; i < duplicates.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << duplicates[i];
                }
                throw UserError(oss.str());
            }
            
            // Add to global set and tools list
            tool_names.insert(server_tool_names.begin(), server_tool_names.end());
            all_tools.insert(all_tools.end(), server_tools.begin(), server_tools.end());
        }
        
        return all_tools;
    });
}

std::future<std::vector<std::shared_ptr<openai_agents::Tool>>> MCPUtil::get_function_tools(
    const std::shared_ptr<MCPServer>& server,
    bool convert_schemas_to_strict,
    const RunContextWrapper& run_context,
    const std::shared_ptr<AgentBase>& agent
) {
    return std::async(std::launch::async, [server, convert_schemas_to_strict, run_context, agent]() 
        -> std::vector<std::shared_ptr<openai_agents::Tool>> {
        
        // Start tracing span for MCP tools
        auto span = tracing::create_mcp_tools_span(server->name());
        
        auto mcp_tools = server->list_tools(run_context, agent).get();
        
        // Record tool names in span
        std::vector<std::string> tool_names;
        for (const auto& tool : mcp_tools) {
            tool_names.push_back(tool.name);
        }
        span->set_result(tool_names);
        
        // Convert MCP tools to function tools
        std::vector<std::shared_ptr<openai_agents::Tool>> function_tools;
        for (const auto& tool : mcp_tools) {
            function_tools.push_back(to_function_tool(tool, server, convert_schemas_to_strict));
        }
        
        return function_tools;
    });
}

std::shared_ptr<FunctionTool> MCPUtil::to_function_tool(
    const Tool& tool,
    const std::shared_ptr<MCPServer>& server,
    bool convert_schemas_to_strict
) {
    auto schema = tool.input_schema;
    bool is_strict = false;

    // MCP spec doesn't require inputSchema to have 'properties', but OpenAI spec does
    if (schema.find("properties") == schema.end()) {
        schema["properties"] = std::unordered_map<std::string, std::any>{};
    }

    // Convert to strict schema if requested
    if (convert_schemas_to_strict) {
        try {
            schema = strict_schema::ensure_strict_json_schema(schema);
            is_strict = true;
        } catch (const std::exception& e) {
            logger::info("Error converting MCP schema to strict mode: " + std::string(e.what()));
        }
    }

    // Create invocation function
    auto invoke_func = [server, tool](const RunContextWrapper& context, const std::string& input_json) -> std::future<std::string> {
        return invoke_mcp_tool(server, tool, context, input_json);
    };

    return std::make_shared<FunctionTool>(
        tool.name,
        tool.description,
        schema,
        invoke_func,
        is_strict
    );
}

std::future<std::string> MCPUtil::invoke_mcp_tool(
    const std::shared_ptr<MCPServer>& server,
    const Tool& tool,
    const RunContextWrapper& context,
    const std::string& input_json
) {
    return std::async(std::launch::async, [server, tool, context, input_json]() -> std::string {
        try {
            auto json_data = parse_json_input(input_json, tool.name);
            
            logger::debug("Invoking MCP tool " + tool.name);
            
            auto result = server->call_tool(tool.name, json_data).get();
            
            logger::debug("MCP tool " + tool.name + " completed");
            
            auto output = format_tool_output(result, server);
            
            // Record in tracing span if available
            auto current_span = tracing::get_current_span();
            if (current_span) {
                current_span->set_output(output);
                current_span->set_mcp_data({{"server", server->name()}});
            }
            
            return output;
            
        } catch (const std::exception& e) {
            logger::error("Error invoking MCP tool " + tool.name + ": " + std::string(e.what()));
            throw AgentsException("Error invoking MCP tool " + tool.name + ": " + std::string(e.what()));
        }
    });
}

std::unordered_map<std::string, std::any> MCPUtil::parse_json_input(
    const std::string& input_json,
    const std::string& tool_name
) {
    try {
        if (input_json.empty()) {
            return {};
        }
        return util::parse_json(input_json);
    } catch (const std::exception& e) {
        logger::debug("Invalid JSON input for tool " + tool_name);
        throw ModelBehaviorError("Invalid JSON input for tool " + tool_name + ": " + input_json);
    }
}

std::string MCPUtil::format_tool_output(
    const CallToolResult& result,
    const std::shared_ptr<MCPServer>& server
) {
    // Handle different content scenarios
    std::string tool_output;
    
    if (result.content.size() == 1) {
        tool_output = result.content[0].to_json();
        
        // Append structured content if available and enabled
        if (server->use_structured_content && result.structured_content) {
            tool_output += "\n" + util::to_json(*result.structured_content);
        }
    } else if (result.content.size() > 1) {
        std::vector<std::string> content_items;
        for (const auto& item : result.content) {
            content_items.push_back(item.to_json());
        }
        
        if (server->use_structured_content && result.structured_content) {
            content_items.push_back(util::to_json(*result.structured_content));
        }
        
        tool_output = util::to_json(content_items);
    } else if (server->use_structured_content && result.structured_content) {
        tool_output = util::to_json(*result.structured_content);
    } else {
        // Empty content is valid (e.g., "no results found")
        tool_output = "[]";
    }
    
    return tool_output;
}

// Utility namespace implementations
namespace utils {

bool is_valid_tool_name(const std::string& name) {
    if (name.empty() || name.length() > 64) {
        return false;
    }
    
    // Check for valid characters: alphanumeric, underscore, hyphen
    std::regex valid_regex("^[a-zA-Z0-9_-]+$");
    return std::regex_match(name, valid_regex);
}

std::string sanitize_tool_name(const std::string& name) {
    std::string sanitized = name;
    
    // Replace invalid characters with underscores
    std::regex invalid_chars("[^a-zA-Z0-9_-]");
    sanitized = std::regex_replace(sanitized, invalid_chars, "_");
    
    // Ensure it starts with letter or underscore
    if (!sanitized.empty() && std::isdigit(sanitized[0])) {
        sanitized = "_" + sanitized;
    }
    
    // Truncate if too long
    if (sanitized.length() > 64) {
        sanitized = sanitized.substr(0, 64);
    }
    
    // Ensure not empty
    if (sanitized.empty()) {
        sanitized = "tool";
    }
    
    return sanitized;
}

bool are_tools_compatible(const Tool& tool1, const Tool& tool2) {
    // Tools are compatible if they have the same name and similar schemas
    if (tool1.name != tool2.name) {
        return false;
    }
    
    // Simple schema compatibility check
    // In practice, this would be more sophisticated
    return tool1.input_schema.size() == tool2.input_schema.size();
}

std::unordered_map<std::string, std::any> merge_tool_schemas(
    const std::vector<std::unordered_map<std::string, std::any>>& schemas
) {
    std::unordered_map<std::string, std::any> merged;
    
    if (schemas.empty()) {
        return merged;
    }
    
    // Start with first schema
    merged = schemas[0];
    
    // Merge properties from other schemas
    for (size_t i = 1; i < schemas.size(); ++i) {
        const auto& schema = schemas[i];
        
        // Merge properties if they exist
        auto merged_props_it = merged.find("properties");
        auto schema_props_it = schema.find("properties");
        
        if (merged_props_it != merged.end() && schema_props_it != schema.end()) {
            try {
                auto& merged_props = std::any_cast<std::unordered_map<std::string, std::any>&>(merged_props_it->second);
                const auto& schema_props = std::any_cast<const std::unordered_map<std::string, std::any>&>(schema_props_it->second);
                
                for (const auto& [key, value] : schema_props) {
                    merged_props[key] = value;
                }
            } catch (const std::bad_any_cast&) {
                // Skip if cast fails
            }
        }
    }
    
    return merged;
}

std::unordered_map<std::string, std::string> extract_tool_metadata(const Tool& tool) {
    std::unordered_map<std::string, std::string> metadata;
    
    metadata["name"] = tool.name;
    metadata["description"] = tool.description;
    
    // Extract basic schema info
    if (tool.input_schema.find("type") != tool.input_schema.end()) {
        try {
            const auto& type_any = tool.input_schema.at("type");
            if (type_any.type() == typeid(std::string)) {
                metadata["schema_type"] = std::any_cast<std::string>(type_any);
            }
        } catch (const std::bad_any_cast&) {
            // Ignore if cast fails
        }
    }
    
    // Count properties
    if (tool.input_schema.find("properties") != tool.input_schema.end()) {
        try {
            const auto& props_any = tool.input_schema.at("properties");
            if (props_any.type() == typeid(std::unordered_map<std::string, std::any>)) {
                const auto& props = std::any_cast<const std::unordered_map<std::string, std::any>&>(props_any);
                metadata["property_count"] = std::to_string(props.size());
            }
        } catch (const std::bad_any_cast&) {
            // Ignore if cast fails
        }
    }
    
    return metadata;
}

} // namespace utils

} // namespace mcp
} // namespace openai_agents