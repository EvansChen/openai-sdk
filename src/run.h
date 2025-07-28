#pragma once

/**
 * Main agent run execution
 */

#include "run_context.h"
#include "result.h"
#include "items.h"
#include <memory>
#include <vector>
#include <functional>
#include <future>

namespace openai_agents {

// Forward declarations
class Agent;
class RunContext;

// Run options
struct RunOptions {
    size_t max_turns = 10;
    bool stream = false;
    bool include_usage = true;
    bool debug = false;
    std::map<std::string, std::any> model_options;
    std::vector<std::string> tool_names;
    std::map<std::string, std::any> metadata;
};

// Run result
struct RunResult {
    bool success;
    std::vector<std::shared_ptr<Item>> messages;
    std::shared_ptr<Usage> usage;
    std::optional<std::string> error_message;
    std::chrono::milliseconds duration;
    size_t turns_taken;
    std::map<std::string, std::any> metadata;
};

// Streaming callback types
using StreamingCallback = std::function<void(const std::shared_ptr<Item>&)>;
using ProgressCallback = std::function<void(size_t current_turn, size_t max_turns)>;

// Main run class
class Run {
private:
    std::shared_ptr<Agent> agent_;
    std::shared_ptr<RunContext> context_;
    RunOptions options_;
    bool is_running_;
    std::future<RunResult> run_future_;

public:
    Run(std::shared_ptr<Agent> agent, const RunOptions& options = {});
    
    // Synchronous execution
    RunResult execute(const std::vector<std::shared_ptr<Item>>& initial_messages);
    RunResult execute(const std::string& prompt);
    
    // Asynchronous execution
    std::future<RunResult> execute_async(const std::vector<std::shared_ptr<Item>>& initial_messages);
    std::future<RunResult> execute_async(const std::string& prompt);
    
    // Streaming execution
    RunResult execute_stream(
        const std::vector<std::shared_ptr<Item>>& initial_messages,
        StreamingCallback callback,
        ProgressCallback progress_callback = nullptr
    );
    
    // Control
    void cancel(const std::string& reason = "User cancelled");
    bool is_running() const { return is_running_; }
    
    // Status
    std::shared_ptr<RunContext> get_context() const { return context_; }
    const RunOptions& get_options() const { return options_; }
    
    // Configuration
    void set_max_turns(size_t max_turns) { options_.max_turns = max_turns; }
    void set_streaming(bool stream) { options_.stream = stream; }
    void add_tool_filter(const std::string& tool_name) { options_.tool_names.push_back(tool_name); }
    void set_model_option(const std::string& key, const std::any& value) { options_.model_options[key] = value; }

private:
    // Internal execution logic
    RunResult run_internal(const std::vector<std::shared_ptr<Item>>& initial_messages);
    bool should_continue(size_t current_turn) const;
    void validate_initial_messages(const std::vector<std::shared_ptr<Item>>& messages) const;
    
    // Turn execution
    struct TurnResult {
        bool success;
        std::vector<std::shared_ptr<Item>> new_messages;
        bool should_continue;
        std::optional<std::string> error_message;
    };
    
    TurnResult execute_turn(size_t turn_number);
    void handle_tool_calls(const std::vector<std::shared_ptr<Item>>& tool_call_items, 
                          std::vector<std::shared_ptr<Item>>& response_items);
    
    // Streaming support
    void emit_streaming_item(const std::shared_ptr<Item>& item, StreamingCallback callback) const;
    void emit_progress(size_t current_turn, size_t max_turns, ProgressCallback callback) const;
};

// Run factory for convenience
class RunFactory {
public:
    static std::shared_ptr<Run> create(std::shared_ptr<Agent> agent, const RunOptions& options = {});
    static std::shared_ptr<Run> create_streaming(std::shared_ptr<Agent> agent, StreamingCallback callback);
    static std::shared_ptr<Run> create_with_max_turns(std::shared_ptr<Agent> agent, size_t max_turns);
    static std::shared_ptr<Run> create_debug(std::shared_ptr<Agent> agent);
    
private:
    static RunOptions create_default_options();
    static RunOptions create_streaming_options(StreamingCallback callback);
    static RunOptions create_debug_options();
};

// Convenience functions
RunResult run_agent(std::shared_ptr<Agent> agent, const std::string& prompt, const RunOptions& options = {});
RunResult run_agent(std::shared_ptr<Agent> agent, const std::vector<std::shared_ptr<Item>>& messages, const RunOptions& options = {});

std::future<RunResult> run_agent_async(std::shared_ptr<Agent> agent, const std::string& prompt, const RunOptions& options = {});

// Batch execution
struct BatchRunResult {
    std::vector<RunResult> results;
    bool all_successful;
    std::chrono::milliseconds total_duration;
    std::shared_ptr<Usage> combined_usage;
};

BatchRunResult run_batch(
    std::shared_ptr<Agent> agent,
    const std::vector<std::string>& prompts,
    const RunOptions& options = {},
    size_t max_concurrent = 5
);

BatchRunResult run_batch(
    std::shared_ptr<Agent> agent,
    const std::vector<std::vector<std::shared_ptr<Item>>>& message_sets,
    const RunOptions& options = {},
    size_t max_concurrent = 5
);

} // namespace openai_agents