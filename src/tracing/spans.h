#pragma once

/**
 * Tracing and observability system
 */

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <any>
#include <functional>

namespace openai_agents {
namespace tracing {

// Forward declarations
class Span;
class Trace;

// Span context for distributed tracing
struct SpanContext {
    std::string trace_id;
    std::string span_id;
    std::string parent_span_id;
    std::map<std::string, std::string> baggage;
    bool sampled;
    
    SpanContext(const std::string& trace_id = "", const std::string& span_id = "");
    std::string to_string() const;
};

// Span data
struct SpanData {
    std::string operation_name;
    SpanContext context;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::map<std::string, std::any> tags;
    std::vector<std::string> logs;
    std::string status;
    std::optional<std::string> error_message;
    
    std::chrono::milliseconds duration() const;
    bool is_finished() const;
};

// Span interface
class Span {
private:
    SpanData data_;
    bool finished_;

public:
    Span(const std::string& operation_name, const SpanContext& context = SpanContext());
    
    // Lifecycle
    void finish();
    void finish(std::chrono::system_clock::time_point end_time);
    
    // Tags
    void set_tag(const std::string& key, const std::any& value);
    std::any get_tag(const std::string& key) const;
    bool has_tag(const std::string& key) const;
    
    // Logs
    void log(const std::string& message);
    void log(const std::string& message, std::chrono::system_clock::time_point timestamp);
    
    // Status
    void set_status(const std::string& status);
    void set_error(const std::string& error_message);
    
    // Context
    const SpanContext& get_context() const { return data_.context; }
    const SpanData& get_data() const { return data_; }
    
    // State
    bool is_finished() const { return finished_; }
    std::chrono::milliseconds duration() const { return data_.duration(); }
};

// Tracer interface
class Tracer {
public:
    virtual ~Tracer() = default;
    
    // Span creation
    virtual std::shared_ptr<Span> start_span(const std::string& operation_name) = 0;
    virtual std::shared_ptr<Span> start_span(const std::string& operation_name, const SpanContext& parent_context) = 0;
    virtual std::shared_ptr<Span> start_child_span(const std::string& operation_name, std::shared_ptr<Span> parent) = 0;
    
    // Context management
    virtual SpanContext extract_context(const std::map<std::string, std::string>& carriers) = 0;
    virtual void inject_context(const SpanContext& context, std::map<std::string, std::string>& carriers) = 0;
    
    // Configuration
    virtual void set_sampling_rate(double rate) = 0;
    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual bool is_enabled() const = 0;
};

// Default tracer implementation
class DefaultTracer : public Tracer {
private:
    bool enabled_;
    double sampling_rate_;
    std::vector<std::shared_ptr<Span>> active_spans_;

public:
    DefaultTracer(bool enabled = true, double sampling_rate = 1.0);
    
    // Tracer interface
    std::shared_ptr<Span> start_span(const std::string& operation_name) override;
    std::shared_ptr<Span> start_span(const std::string& operation_name, const SpanContext& parent_context) override;
    std::shared_ptr<Span> start_child_span(const std::string& operation_name, std::shared_ptr<Span> parent) override;
    
    SpanContext extract_context(const std::map<std::string, std::string>& carriers) override;
    void inject_context(const SpanContext& context, std::map<std::string, std::string>& carriers) override;
    
    void set_sampling_rate(double rate) override { sampling_rate_ = rate; }
    void enable() override { enabled_ = true; }
    void disable() override { enabled_ = false; }
    bool is_enabled() const override { return enabled_; }
    
    // Management
    std::vector<std::shared_ptr<Span>> get_active_spans() const { return active_spans_; }
    void clear_active_spans() { active_spans_.clear(); }

private:
    std::string generate_trace_id() const;
    std::string generate_span_id() const;
    bool should_sample() const;
};

// Trace processor interface
class TraceProcessor {
public:
    virtual ~TraceProcessor() = default;
    virtual void process_span(std::shared_ptr<Span> span) = 0;
    virtual void flush() = 0;
};

// Console processor
class ConsoleTraceProcessor : public TraceProcessor {
private:
    bool include_tags_;
    bool include_logs_;

public:
    ConsoleTraceProcessor(bool include_tags = true, bool include_logs = true);
    void process_span(std::shared_ptr<Span> span) override;
    void flush() override;

private:
    std::string format_span(std::shared_ptr<Span> span) const;
};

// Memory processor (for testing)
class MemoryTraceProcessor : public TraceProcessor {
private:
    std::vector<std::shared_ptr<Span>> spans_;
    size_t max_spans_;

public:
    MemoryTraceProcessor(size_t max_spans = 1000);
    void process_span(std::shared_ptr<Span> span) override;
    void flush() override;
    
    std::vector<std::shared_ptr<Span>> get_spans() const { return spans_; }
    void clear() { spans_.clear(); }
};

// File processor
class FileTraceProcessor : public TraceProcessor {
private:
    std::string filename_;
    std::ofstream file_;

public:
    FileTraceProcessor(const std::string& filename);
    ~FileTraceProcessor();
    
    void process_span(std::shared_ptr<Span> span) override;
    void flush() override;

private:
    std::string span_to_json(std::shared_ptr<Span> span) const;
};

// Tracing manager
class TracingManager {
private:
    std::shared_ptr<Tracer> tracer_;
    std::vector<std::shared_ptr<TraceProcessor>> processors_;
    bool auto_finish_;

public:
    TracingManager();
    
    // Tracer management
    void set_tracer(std::shared_ptr<Tracer> tracer);
    std::shared_ptr<Tracer> get_tracer() const { return tracer_; }
    
    // Processor management
    void add_processor(std::shared_ptr<TraceProcessor> processor);
    void remove_processor(std::shared_ptr<TraceProcessor> processor);
    void clear_processors();
    
    // Span processing
    void process_span(std::shared_ptr<Span> span);
    void flush_all();
    
    // Configuration
    void set_auto_finish(bool auto_finish) { auto_finish_ = auto_finish; }
    bool get_auto_finish() const { return auto_finish_; }
    
    // Convenience methods
    void setup_console_tracing(bool include_tags = true, bool include_logs = true);
    void setup_file_tracing(const std::string& filename);
    void setup_memory_tracing(size_t max_spans = 1000);
};

// Global tracing
TracingManager& get_global_tracing_manager();
std::shared_ptr<Tracer> get_global_tracer();
void setup_tracing(std::shared_ptr<Tracer> tracer = nullptr);

// RAII span wrapper
class ScopedSpan {
private:
    std::shared_ptr<Span> span_;
    bool auto_finish_;

public:
    ScopedSpan(const std::string& operation_name, bool auto_finish = true);
    ScopedSpan(std::shared_ptr<Span> span, bool auto_finish = true);
    ~ScopedSpan();
    
    std::shared_ptr<Span> get_span() const { return span_; }
    void finish();
    
    // Delegate to span
    void set_tag(const std::string& key, const std::any& value) { span_->set_tag(key, value); }
    void log(const std::string& message) { span_->log(message); }
    void set_error(const std::string& error_message) { span_->set_error(error_message); }
};

// Tracing macros
#define TRACE_SPAN(name) ScopedSpan __trace_span(name)
#define TRACE_SPAN_WITH_TAGS(name, tags) \
    ScopedSpan __trace_span(name); \
    for (const auto& [key, value] : tags) { \
        __trace_span.set_tag(key, value); \
    }

} // namespace tracing
} // namespace openai_agents