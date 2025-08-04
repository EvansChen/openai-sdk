#include "visualization.h"
#include <fstream>
#include <algorithm>
#include <regex>
#include <queue>

namespace openai_agents {
namespace extensions {

std::string get_main_graph(
    const std::shared_ptr<AgentBase>& agent,
    const GraphConfig& config
) {
    std::ostringstream oss;
    
    // Graph header
    oss << "digraph G {\n"
        << "    graph [splines=true];\n"
        << "    node [fontname=\"" << config.node_font << "\"];\n"
        << "    edge [penwidth=" << config.edge_width << "];\n\n";
    
    // Generate nodes and edges
    std::unordered_set<std::string> visited;
    oss << get_all_nodes(agent, nullptr, &visited, config);
    
    visited.clear();
    oss << get_all_edges(agent, nullptr, &visited, config);
    
    oss << "}\n";
    
    return oss.str();
}

std::string get_all_nodes(
    const std::shared_ptr<AgentBase>& agent,
    const std::shared_ptr<AgentBase>& parent,
    std::unordered_set<std::string>* visited,
    const GraphConfig& config
) {
    if (!agent || !visited) {
        return "";
    }
    
    if (visited->count(agent->get_name()) > 0) {
        return "";
    }
    visited->insert(agent->get_name());
    
    std::ostringstream oss;
    
    // Add start/end nodes for root agent
    if (!parent && config.show_start_end) {
        oss << "    \"__start__\" [label=\"start\", shape=ellipse, style=filled, "
            << "fillcolor=" << config.start_end_color << ", "
            << "width=" << config.tool_width << ", height=" << config.tool_height << "];\n";
        oss << "    \"__end__\" [label=\"end\", shape=ellipse, style=filled, "
            << "fillcolor=" << config.start_end_color << ", "
            << "width=" << config.tool_width << ", height=" << config.tool_height << "];\n";
    }
    
    // Add agent node
    std::string agent_style = config.use_rounded_agents ? "filled,rounded" : "filled";
    oss << "    \"" << detail::escape_dot_label(agent->get_name()) 
        << "\" [label=\"" << detail::escape_dot_label(agent->get_name()) 
        << "\", shape=box, style=" << agent_style << ", "
        << "fillcolor=" << config.agent_color << ", "
        << "width=" << config.agent_width << ", height=" << config.agent_height << "];\n";
    
    // Add tool nodes
    auto tools = agent->get_tools();
    for (const auto& tool : tools) {
        oss << "    \"" << detail::escape_dot_label(tool->get_name()) 
            << "\" [label=\"" << detail::escape_dot_label(tool->get_name()) 
            << "\", shape=ellipse, style=filled, "
            << "fillcolor=" << config.tool_color << ", "
            << "width=" << config.tool_width << ", height=" << config.tool_height << "];\n";
    }
    
    // Add handoff target nodes and recurse
    auto handoffs = agent->get_handoffs();
    for (const auto& handoff : handoffs) {
        std::string target_name = handoff->get_target_agent_name();
        
        if (visited->count(target_name) == 0) {
            oss << "    \"" << detail::escape_dot_label(target_name) 
                << "\" [label=\"" << detail::escape_dot_label(target_name) 
                << "\", shape=box, style=" << agent_style << ", "
                << "fillcolor=" << config.agent_color << ", "
                << "width=" << config.agent_width << ", height=" << config.agent_height << "];\n";
            
            // If we have the actual agent object, recurse
            auto target_agent = handoff->get_target_agent();
            if (target_agent) {
                oss << get_all_nodes(target_agent, agent, visited, config);
            }
        }
    }
    
    return oss.str();
}

std::string get_all_edges(
    const std::shared_ptr<AgentBase>& agent,
    const std::shared_ptr<AgentBase>& parent,
    std::unordered_set<std::string>* visited,
    const GraphConfig& config
) {
    if (!agent || !visited) {
        return "";
    }
    
    if (visited->count(agent->get_name()) > 0) {
        return "";
    }
    visited->insert(agent->get_name());
    
    std::ostringstream oss;
    
    // Add edge from start to root agent
    if (!parent && config.show_start_end) {
        oss << "    \"__start__\" -> \"" << detail::escape_dot_label(agent->get_name()) << "\";\n";
    }
    
    // Add tool edges (bidirectional, dotted)
    auto tools = agent->get_tools();
    for (const auto& tool : tools) {
        oss << "    \"" << detail::escape_dot_label(agent->get_name()) 
            << "\" -> \"" << detail::escape_dot_label(tool->get_name()) 
            << "\" [style=dotted];\n";
        oss << "    \"" << detail::escape_dot_label(tool->get_name()) 
            << "\" -> \"" << detail::escape_dot_label(agent->get_name()) 
            << "\" [style=dotted];\n";
    }
    
    // Add handoff edges
    auto handoffs = agent->get_handoffs();
    bool has_handoffs = false;
    for (const auto& handoff : handoffs) {
        std::string target_name = handoff->get_target_agent_name();
        oss << "    \"" << detail::escape_dot_label(agent->get_name()) 
            << "\" -> \"" << detail::escape_dot_label(target_name) << "\";\n";
        has_handoffs = true;
        
        // Recurse to target agent
        auto target_agent = handoff->get_target_agent();
        if (target_agent) {
            oss << get_all_edges(target_agent, agent, visited, config);
        }
    }
    
    // Add edge to end if no handoffs
    if (!has_handoffs && config.show_start_end) {
        oss << "    \"" << detail::escape_dot_label(agent->get_name()) << "\" -> \"__end__\";\n";
    }
    
    return oss.str();
}

std::string get_agent_graph(
    const std::shared_ptr<AgentBase>& agent,
    const GraphConfig& config
) {
    GraphConfig agent_config = config;
    GraphBuilder builder(agent_config);
    return builder.set_root_agent(agent)
                 .include_tools(false)
                 .include_handoffs(true)
                 .build();
}

std::string get_tool_graph(
    const std::shared_ptr<AgentBase>& agent,
    const GraphConfig& config
) {
    GraphConfig tool_config = config;
    GraphBuilder builder(tool_config);
    return builder.set_root_agent(agent)
                 .include_tools(true)
                 .include_handoffs(false)
                 .build();
}

bool save_dot_file(const std::string& dot_content, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << dot_content;
    file.close();
    
    return true;
}

bool draw_graph(
    const std::shared_ptr<AgentBase>& agent,
    const std::string& filename,
    const GraphConfig& config
) {
    if (!detail::is_graphviz_available()) {
        return false;
    }
    
    std::string dot_content = get_main_graph(agent, config);
    std::string dot_filename = filename + ".dot";
    std::string png_filename = filename + ".png";
    
    if (!save_dot_file(dot_content, dot_filename)) {
        return false;
    }
    
    return detail::execute_graphviz(dot_filename, png_filename);
}

// GraphBuilder implementation
GraphBuilder::GraphBuilder(const GraphConfig& config) : config_(config) {}

GraphBuilder& GraphBuilder::set_root_agent(const std::shared_ptr<AgentBase>& agent) {
    root_agent_ = agent;
    return *this;
}

GraphBuilder& GraphBuilder::add_custom_node(const std::string& name, const std::string& label, 
                                           const std::string& color) {
    custom_nodes_.emplace_back(name, label, color);
    return *this;
}

GraphBuilder& GraphBuilder::add_custom_edge(const std::string& from, const std::string& to,
                                           const std::string& style) {
    custom_edges_.emplace_back(from, to, style);
    return *this;
}

GraphBuilder& GraphBuilder::include_tools(bool include) {
    include_tools_ = include;
    return *this;
}

GraphBuilder& GraphBuilder::include_handoffs(bool include) {
    include_handoffs_ = include;
    return *this;
}

GraphBuilder& GraphBuilder::set_max_depth(int depth) {
    max_depth_ = depth;
    return *this;
}

GraphBuilder& GraphBuilder::filter_agents(const std::string& pattern) {
    agent_filter_ = pattern;
    return *this;
}

std::string GraphBuilder::build() {
    if (!root_agent_) {
        return "";
    }
    
    std::ostringstream oss;
    
    // Graph header
    oss << "digraph G {\n"
        << "    graph [splines=true];\n"
        << "    node [fontname=\"" << config_.node_font << "\"];\n"
        << "    edge [penwidth=" << config_.edge_width << "];\n\n";
    
    // Build the graph with options
    std::unordered_set<std::string> visited;
    
    // Custom implementation respecting builder options
    // This would implement the filtering and depth limiting logic
    
    // Add custom nodes
    for (const auto& [name, label, color] : custom_nodes_) {
        oss << "    \"" << detail::escape_dot_label(name) 
            << "\" [label=\"" << detail::escape_dot_label(label) 
            << "\", style=filled, fillcolor=" << color << "];\n";
    }
    
    // Add custom edges
    for (const auto& [from, to, style] : custom_edges_) {
        oss << "    \"" << detail::escape_dot_label(from) 
            << "\" -> \"" << detail::escape_dot_label(to) 
            << "\" [style=" << style << "];\n";
    }
    
    oss << "}\n";
    
    return oss.str();
}

// Analysis namespace implementation
namespace analysis {

size_t count_agents(const std::shared_ptr<AgentBase>& root_agent) {
    if (!root_agent) return 0;
    
    std::unordered_set<std::string> visited;
    std::queue<std::shared_ptr<AgentBase>> queue;
    queue.push(root_agent);
    
    while (!queue.empty()) {
        auto agent = queue.front();
        queue.pop();
        
        if (!agent || visited.count(agent->get_name()) > 0) {
            continue;
        }
        
        visited.insert(agent->get_name());
        
        auto handoffs = agent->get_handoffs();
        for (const auto& handoff : handoffs) {
            auto target = handoff->get_target_agent();
            if (target) {
                queue.push(target);
            }
        }
    }
    
    return visited.size();
}

size_t count_tools(const std::shared_ptr<AgentBase>& root_agent) {
    if (!root_agent) return 0;
    
    std::unordered_set<std::string> visited_agents;
    std::unordered_set<std::string> all_tools;
    std::queue<std::shared_ptr<AgentBase>> queue;
    queue.push(root_agent);
    
    while (!queue.empty()) {
        auto agent = queue.front();
        queue.pop();
        
        if (!agent || visited_agents.count(agent->get_name()) > 0) {
            continue;
        }
        
        visited_agents.insert(agent->get_name());
        
        auto tools = agent->get_tools();
        for (const auto& tool : tools) {
            all_tools.insert(tool->get_name());
        }
        
        auto handoffs = agent->get_handoffs();
        for (const auto& handoff : handoffs) {
            auto target = handoff->get_target_agent();
            if (target) {
                queue.push(target);
            }
        }
    }
    
    return all_tools.size();
}

size_t count_handoffs(const std::shared_ptr<AgentBase>& root_agent) {
    if (!root_agent) return 0;
    
    std::unordered_set<std::string> visited;
    std::queue<std::shared_ptr<AgentBase>> queue;
    queue.push(root_agent);
    size_t handoff_count = 0;
    
    while (!queue.empty()) {
        auto agent = queue.front();
        queue.pop();
        
        if (!agent || visited.count(agent->get_name()) > 0) {
            continue;
        }
        
        visited.insert(agent->get_name());
        
        auto handoffs = agent->get_handoffs();
        handoff_count += handoffs.size();
        
        for (const auto& handoff : handoffs) {
            auto target = handoff->get_target_agent();
            if (target) {
                queue.push(target);
            }
        }
    }
    
    return handoff_count;
}

NetworkStats analyze_network(const std::shared_ptr<AgentBase>& root_agent) {
    NetworkStats stats;
    
    if (!root_agent) {
        return stats;
    }
    
    stats.agent_count = count_agents(root_agent);
    stats.tool_count = count_tools(root_agent);
    stats.handoff_count = count_handoffs(root_agent);
    stats.max_depth = calculate_max_depth(root_agent);
    stats.has_cycles = has_cycles(root_agent);
    stats.agent_names = get_all_agent_names(root_agent);
    stats.tool_names = get_all_tool_names(root_agent);
    
    return stats;
}

} // namespace analysis

// Detail namespace implementation
namespace detail {

std::string escape_dot_label(const std::string& label) {
    std::string escaped = label;
    
    // Escape double quotes
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    // Escape backslashes
    pos = 0;
    while ((pos = escaped.find('\\', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    return escaped;
}

std::string generate_node_id(const std::string& name, const std::string& type) {
    std::string id = name;
    if (!type.empty()) {
        id += "_" + type;
    }
    
    // Replace spaces and special characters with underscores
    std::regex special_chars("[^a-zA-Z0-9_]");
    id = std::regex_replace(id, special_chars, "_");
    
    return id;
}

bool is_graphviz_available() {
    // Simple check - try to run "dot --version"
    return system("dot --version > /dev/null 2>&1") == 0;
}

bool execute_graphviz(const std::string& dot_file, const std::string& output_file, 
                     const std::string& format) {
    std::string command = "dot -T" + format + " \"" + dot_file + "\" -o \"" + output_file + "\"";
    return system(command.c_str()) == 0;
}

} // namespace detail

} // namespace extensions
} // namespace openai_agents