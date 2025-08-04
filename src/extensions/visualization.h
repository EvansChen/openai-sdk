#pragma once

/**
 * Visualization utilities for agent graphs and handoff structures
 * 
 * This module provides functionality to generate visual representations
 * of agent networks, including their tools and handoff relationships.
 * Output is generated in DOT format for use with Graphviz.
 */

#include "../agent.h"
#include "../handoffs.h"
#include "../tool.h"
#include <string>
#include <memory>
#include <unordered_set>
#include <vector>
#include <sstream>

namespace openai_agents {
namespace extensions {

/**
 * Configuration options for graph visualization
 */
struct GraphConfig {
    std::string node_font = "Arial";                    ///< Font for node labels
    std::string agent_color = "lightyellow";           ///< Fill color for agent nodes
    std::string tool_color = "lightgreen";             ///< Fill color for tool nodes
    std::string start_end_color = "lightblue";         ///< Fill color for start/end nodes
    double agent_width = 1.5;                          ///< Width of agent nodes
    double agent_height = 0.8;                         ///< Height of agent nodes
    double tool_width = 0.5;                           ///< Width of tool nodes
    double tool_height = 0.3;                          ///< Height of tool nodes
    double edge_width = 1.5;                           ///< Width of edges
    bool show_start_end = true;                        ///< Whether to show start/end nodes
    bool use_rounded_agents = true;                    ///< Whether to use rounded rectangles for agents
};

/**
 * Generate the main graph structure in DOT format for the given agent
 * 
 * This creates a complete Graphviz DOT representation of an agent network,
 * including all nodes (agents, tools) and edges (connections, handoffs).
 * 
 * @param agent The root agent for which the graph is generated
 * @param config Configuration options for the graph appearance
 * @return DOT format string representing the complete graph
 * 
 * @example
 * ```cpp
 * auto agent = create_agent("main_agent");
 * std::string dot_graph = get_main_graph(agent);
 * // Use with graphviz: dot -Tpng -o graph.png graph.dot
 * ```
 */
std::string get_main_graph(
    const std::shared_ptr<AgentBase>& agent,
    const GraphConfig& config = GraphConfig{}
);

/**
 * Recursively generate nodes for the given agent and its handoffs in DOT format
 * 
 * @param agent The agent for which nodes are generated
 * @param parent The parent agent (nullptr for root)
 * @param visited Set of visited agent names to prevent cycles
 * @param config Configuration options for node appearance
 * @return DOT format string representing the nodes
 */
std::string get_all_nodes(
    const std::shared_ptr<AgentBase>& agent,
    const std::shared_ptr<AgentBase>& parent = nullptr,
    std::unordered_set<std::string>* visited = nullptr,
    const GraphConfig& config = GraphConfig{}
);

/**
 * Recursively generate edges for the given agent and its handoffs in DOT format
 * 
 * @param agent The agent for which edges are generated
 * @param parent The parent agent (nullptr for root)
 * @param visited Set of visited agent names to prevent cycles
 * @param config Configuration options for edge appearance
 * @return DOT format string representing the edges
 */
std::string get_all_edges(
    const std::shared_ptr<AgentBase>& agent,
    const std::shared_ptr<AgentBase>& parent = nullptr,
    std::unordered_set<std::string>* visited = nullptr,
    const GraphConfig& config = GraphConfig{}
);

/**
 * Generate a simplified graph showing only agent-to-agent relationships
 * 
 * @param agent The root agent
 * @param config Configuration options
 * @return DOT format string for agent-only graph
 */
std::string get_agent_graph(
    const std::shared_ptr<AgentBase>& agent,
    const GraphConfig& config = GraphConfig{}
);

/**
 * Generate a tool-focused graph showing agent-tool relationships
 * 
 * @param agent The root agent
 * @param config Configuration options
 * @return DOT format string for tool-focused graph
 */
std::string get_tool_graph(
    const std::shared_ptr<AgentBase>& agent,
    const GraphConfig& config = GraphConfig{}
);

/**
 * Save a DOT graph to a file
 * 
 * @param dot_content The DOT format content
 * @param filename The output filename
 * @return true if successful, false otherwise
 */
bool save_dot_file(const std::string& dot_content, const std::string& filename);

/**
 * Generate and save a PNG image using Graphviz (if available)
 * 
 * @param agent The agent to visualize
 * @param filename The output filename (without extension)
 * @param config Configuration options
 * @return true if successful, false otherwise
 * 
 * @note Requires Graphviz to be installed and accessible via system PATH
 */
bool draw_graph(
    const std::shared_ptr<AgentBase>& agent,
    const std::string& filename,
    const GraphConfig& config = GraphConfig{}
);

/**
 * Graph builder utility class for more complex visualizations
 */
class GraphBuilder {
public:
    explicit GraphBuilder(const GraphConfig& config = GraphConfig{});

    /**
     * Set the root agent for the graph
     */
    GraphBuilder& set_root_agent(const std::shared_ptr<AgentBase>& agent);

    /**
     * Add a custom node to the graph
     */
    GraphBuilder& add_custom_node(const std::string& name, const std::string& label, 
                                 const std::string& color = "white");

    /**
     * Add a custom edge to the graph
     */
    GraphBuilder& add_custom_edge(const std::string& from, const std::string& to,
                                 const std::string& style = "solid");

    /**
     * Set whether to include tool nodes
     */
    GraphBuilder& include_tools(bool include = true);

    /**
     * Set whether to include handoff edges
     */
    GraphBuilder& include_handoffs(bool include = true);

    /**
     * Set maximum depth for traversal
     */
    GraphBuilder& set_max_depth(int depth);

    /**
     * Filter agents by name pattern
     */
    GraphBuilder& filter_agents(const std::string& pattern);

    /**
     * Build and return the DOT graph
     */
    std::string build();

private:
    GraphConfig config_;
    std::shared_ptr<AgentBase> root_agent_;
    std::vector<std::tuple<std::string, std::string, std::string>> custom_nodes_;
    std::vector<std::tuple<std::string, std::string, std::string>> custom_edges_;
    bool include_tools_ = true;
    bool include_handoffs_ = true;
    int max_depth_ = -1;
    std::string agent_filter_;
};

/**
 * Utility functions for graph analysis
 */
namespace analysis {

/**
 * Count the total number of agents in the network
 */
size_t count_agents(const std::shared_ptr<AgentBase>& root_agent);

/**
 * Count the total number of tools in the network
 */
size_t count_tools(const std::shared_ptr<AgentBase>& root_agent);

/**
 * Count the total number of handoffs in the network
 */
size_t count_handoffs(const std::shared_ptr<AgentBase>& root_agent);

/**
 * Find the maximum depth of the agent network
 */
int calculate_max_depth(const std::shared_ptr<AgentBase>& root_agent);

/**
 * Detect cycles in the handoff network
 */
bool has_cycles(const std::shared_ptr<AgentBase>& root_agent);

/**
 * Get all agent names in the network
 */
std::vector<std::string> get_all_agent_names(const std::shared_ptr<AgentBase>& root_agent);

/**
 * Get all tool names in the network
 */
std::vector<std::string> get_all_tool_names(const std::shared_ptr<AgentBase>& root_agent);

/**
 * Generate network statistics
 */
struct NetworkStats {
    size_t agent_count = 0;
    size_t tool_count = 0;
    size_t handoff_count = 0;
    int max_depth = 0;
    bool has_cycles = false;
    std::vector<std::string> agent_names;
    std::vector<std::string> tool_names;
};

NetworkStats analyze_network(const std::shared_ptr<AgentBase>& root_agent);

} // namespace analysis

// Helper functions for internal use
namespace detail {

/**
 * Escape DOT special characters in labels
 */
std::string escape_dot_label(const std::string& label);

/**
 * Generate a unique node ID for DOT format
 */
std::string generate_node_id(const std::string& name, const std::string& type = "");

/**
 * Check if Graphviz is available on the system
 */
bool is_graphviz_available();

/**
 * Execute Graphviz command to generate image
 */
bool execute_graphviz(const std::string& dot_file, const std::string& output_file, 
                     const std::string& format = "png");

} // namespace detail

} // namespace extensions
} // namespace openai_agents