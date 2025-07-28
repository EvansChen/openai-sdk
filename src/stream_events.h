#pragma once

/**
 * Stream events for real-time agent communication
 */

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <any>
#include <chrono>
#include <map>

namespace openai_agents {

// Forward declarations
class Agent;
class Item;
class Usage;

// Event types
enum class StreamEventType {
    RunStart,
    RunComplete,
    RunError,
    StepStart,
    StepComplete,
    MessageDelta,
    MessageComplete,
    ToolCallStart,
    ToolCallComplete,
    ToolCallError,
    UsageUpdate,
    Custom
};

// Base stream event
struct StreamEvent {
    StreamEventType type;
    std::string event_id;
    std::chrono::system_clock::time_point timestamp;
    std::string run_id;
    std::map<std::string, std::any> metadata;
    
    StreamEvent(StreamEventType type, const std::string& run_id = "");
    virtual ~StreamEvent() = default;
    
    // Serialization
    virtual std::map<std::string, std::any> to_dict() const;
    virtual std::string to_json() const;
};

// Run events
struct RunStartEvent : public StreamEvent {
    std::shared_ptr<Agent> agent;
    std::vector<std::shared_ptr<Item>> initial_messages;
    std::map<std::string, std::any> run_options;
    
    RunStartEvent(const std::string& run_id, std::shared_ptr<Agent> agent,
                  const std::vector<std::shared_ptr<Item>>& initial_messages,
                  const std::map<std::string, std::any>& run_options = {});
    
    std::map<std::string, std::any> to_dict() const override;
};

struct RunCompleteEvent : public StreamEvent {
    std::vector<std::shared_ptr<Item>> final_messages;
    std::shared_ptr<Usage> usage;
    size_t total_steps;
    std::chrono::milliseconds duration;
    
    RunCompleteEvent(const std::string& run_id,
                     const std::vector<std::shared_ptr<Item>>& final_messages,
                     std::shared_ptr<Usage> usage,
                     size_t total_steps,
                     std::chrono::milliseconds duration);
    
    std::map<std::string, std::any> to_dict() const override;
};

struct RunErrorEvent : public StreamEvent {
    std::string error_message;
    std::string error_type;
    std::vector<std::shared_ptr<Item>> partial_messages;
    
    RunErrorEvent(const std::string& run_id,
                  const std::string& error_message,
                  const std::string& error_type = "UnknownError",
                  const std::vector<std::shared_ptr<Item>>& partial_messages = {});
    
    std::map<std::string, std::any> to_dict() const override;
};

// Step events
struct StepStartEvent : public StreamEvent {
    size_t step_number;
    std::vector<std::shared_ptr<Item>> input_messages;
    
    StepStartEvent(const std::string& run_id, size_t step_number,
                   const std::vector<std::shared_ptr<Item>>& input_messages);
    
    std::map<std::string, std::any> to_dict() const override;
};

struct StepCompleteEvent : public StreamEvent {
    size_t step_number;
    std::vector<std::shared_ptr<Item>> output_messages;
    std::chrono::milliseconds step_duration;
    
    StepCompleteEvent(const std::string& run_id, size_t step_number,
                      const std::vector<std::shared_ptr<Item>>& output_messages,
                      std::chrono::milliseconds step_duration);
    
    std::map<std::string, std::any> to_dict() const override;
};

// Message events
struct MessageDeltaEvent : public StreamEvent {
    std::shared_ptr<Item> delta_item;
    std::string accumulated_content;
    
    MessageDeltaEvent(const std::string& run_id,
                      std::shared_ptr<Item> delta_item,
                      const std::string& accumulated_content = "");
    
    std::map<std::string, std::any> to_dict() const override;
};

struct MessageCompleteEvent : public StreamEvent {
    std::shared_ptr<Item> complete_message;
    
    MessageCompleteEvent(const std::string& run_id, std::shared_ptr<Item> complete_message);
    
    std::map<std::string, std::any> to_dict() const override;
};

// Tool events
struct ToolCallStartEvent : public StreamEvent {
    std::string tool_call_id;
    std::string tool_name;
    std::map<std::string, std::any> tool_arguments;
    
    ToolCallStartEvent(const std::string& run_id,
                       const std::string& tool_call_id,
                       const std::string& tool_name,
                       const std::map<std::string, std::any>& tool_arguments);
    
    std::map<std::string, std::any> to_dict() const override;
};

struct ToolCallCompleteEvent : public StreamEvent {
    std::string tool_call_id;
    std::string tool_name;
    std::any tool_result;
    std::chrono::milliseconds execution_time;
    
    ToolCallCompleteEvent(const std::string& run_id,
                          const std::string& tool_call_id,
                          const std::string& tool_name,
                          const std::any& tool_result,
                          std::chrono::milliseconds execution_time);
    
    std::map<std::string, std::any> to_dict() const override;
};

struct ToolCallErrorEvent : public StreamEvent {
    std::string tool_call_id;
    std::string tool_name;
    std::string error_message;
    std::string error_type;
    
    ToolCallErrorEvent(const std::string& run_id,
                       const std::string& tool_call_id,
                       const std::string& tool_name,
                       const std::string& error_message,
                       const std::string& error_type = "ToolError");
    
    std::map<std::string, std::any> to_dict() const override;
};

// Usage events
struct UsageUpdateEvent : public StreamEvent {
    std::shared_ptr<Usage> current_usage;
    std::shared_ptr<Usage> delta_usage;
    
    UsageUpdateEvent(const std::string& run_id,
                     std::shared_ptr<Usage> current_usage,
                     std::shared_ptr<Usage> delta_usage = nullptr);
    
    std::map<std::string, std::any> to_dict() const override;
};

// Custom events
struct CustomEvent : public StreamEvent {
    std::string custom_type;
    std::map<std::string, std::any> custom_data;
    
    CustomEvent(const std::string& run_id,
                const std::string& custom_type,
                const std::map<std::string, std::any>& custom_data);
    
    std::map<std::string, std::any> to_dict() const override;
};

// Event handler types
using StreamEventHandler = std::function<void(std::shared_ptr<StreamEvent>)>;
using TypedEventHandler = std::function<void(std::shared_ptr<StreamEvent>)>;

// Stream event emitter
class StreamEventEmitter {
private:
    std::vector<StreamEventHandler> global_handlers_;
    std::map<StreamEventType, std::vector<TypedEventHandler>> typed_handlers_;
    bool enabled_;

public:
    StreamEventEmitter(bool enabled = true) : enabled_(enabled) {}

    // Handler registration
    void add_handler(StreamEventHandler handler);
    void add_typed_handler(StreamEventType type, TypedEventHandler handler);
    void remove_all_handlers();
    void remove_typed_handlers(StreamEventType type);

    // Event emission
    void emit(std::shared_ptr<StreamEvent> event);
    void emit_run_start(const std::string& run_id, std::shared_ptr<Agent> agent,
                       const std::vector<std::shared_ptr<Item>>& initial_messages,
                       const std::map<std::string, std::any>& run_options = {});
    
    void emit_run_complete(const std::string& run_id,
                          const std::vector<std::shared_ptr<Item>>& final_messages,
                          std::shared_ptr<Usage> usage,
                          size_t total_steps,
                          std::chrono::milliseconds duration);
    
    void emit_message_delta(const std::string& run_id,
                           std::shared_ptr<Item> delta_item,
                           const std::string& accumulated_content = "");

    // Control
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }
    bool is_enabled() const { return enabled_; }

    // Statistics
    size_t handler_count() const;
    size_t typed_handler_count(StreamEventType type) const;
};

// Global event emitter
StreamEventEmitter& get_global_event_emitter();

// Event utilities
class StreamEventUtils {
public:
    static std::string event_type_to_string(StreamEventType type);
    static StreamEventType string_to_event_type(const std::string& type_str);
    static std::string generate_event_id();
    static bool is_error_event(StreamEventType type);
    static bool is_completion_event(StreamEventType type);
};

// Event filtering
class EventFilter {
public:
    virtual ~EventFilter() = default;
    virtual bool should_emit(std::shared_ptr<StreamEvent> event) = 0;
};

class TypeFilter : public EventFilter {
private:
    std::vector<StreamEventType> allowed_types_;
public:
    TypeFilter(const std::vector<StreamEventType>& allowed_types) : allowed_types_(allowed_types) {}
    bool should_emit(std::shared_ptr<StreamEvent> event) override;
};

class RunIdFilter : public EventFilter {
private:
    std::string target_run_id_;
public:
    RunIdFilter(const std::string& run_id) : target_run_id_(run_id) {}
    bool should_emit(std::shared_ptr<StreamEvent> event) override;
};

// Filtered emitter
class FilteredStreamEventEmitter {
private:
    StreamEventEmitter& base_emitter_;
    std::vector<std::unique_ptr<EventFilter>> filters_;

public:
    FilteredStreamEventEmitter(StreamEventEmitter& base_emitter) : base_emitter_(base_emitter) {}

    void add_filter(std::unique_ptr<EventFilter> filter);
    void clear_filters();
    void emit(std::shared_ptr<StreamEvent> event);
};

} // namespace openai_agents