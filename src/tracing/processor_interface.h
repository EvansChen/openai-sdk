#pragma once

/**
 * Tracing Processor Interface for OpenAI Agents Framework
 * 
 * This module defines the interface for processing trace and span data,
 * allowing for pluggable backends and custom processing logic.
 */

#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace openai_agents {
namespace tracing {

/**
 * Base interface for trace processors
 * 
 * Processors are responsible for handling trace and span data,
 * such as sending to external systems, logging, or storage.
 */
class TracingProcessor {
public:
    virtual ~TracingProcessor() = default;
    
    /**
     * Process a single span
     * 
     * @param span_data JSON representation of the span data
     */
    virtual void process_span(const nlohmann::json& span_data) = 0;
    
    /**
     * Process a complete trace
     * 
     * @param trace_data JSON representation of the trace data
     */
    virtual void process_trace(const nlohmann::json& trace_data) = 0;
    
    /**
     * Process multiple spans in batch
     * 
     * @param spans_data Vector of JSON span representations
     */
    virtual void process_spans_batch(const std::vector<nlohmann::json>& spans_data) {
        for (const auto& span_data : spans_data) {
            process_span(span_data);
        }
    }
    
    /**
     * Process multiple traces in batch
     * 
     * @param traces_data Vector of JSON trace representations
     */
    virtual void process_traces_batch(const std::vector<nlohmann::json>& traces_data) {
        for (const auto& trace_data : traces_data) {
            process_trace(trace_data);
        }
    }
    
    /**
     * Flush any pending operations
     * 
     * Called when the processor should ensure all data is processed.
     */
    virtual void flush() {}
    
    /**
     * Shutdown the processor
     * 
     * Called when the processor should clean up resources.
     */
    virtual void shutdown() {}
    
    /**
     * Get processor configuration information
     */
    virtual nlohmann::json get_config() const { return nlohmann::json::object(); }
    
    /**
     * Get processor status information
     */
    virtual nlohmann::json get_status() const { return nlohmann::json::object(); }
};

/**
 * Processor configuration options
 */
struct ProcessorConfig {
    std::string name;
    std::string type;
    nlohmann::json settings;
    bool enabled = true;
    
    ProcessorConfig() = default;
    ProcessorConfig(const std::string& name, const std::string& type, 
                   const nlohmann::json& settings = nlohmann::json::object())
        : name(name), type(type), settings(settings) {}
};

/**
 * Processor factory interface
 */
class ProcessorFactory {
public:
    virtual ~ProcessorFactory() = default;
    
    /**
     * Create a processor from configuration
     */
    virtual std::unique_ptr<TracingProcessor> create_processor(const ProcessorConfig& config) = 0;
    
    /**
     * Get supported processor types
     */
    virtual std::vector<std::string> get_supported_types() const = 0;
    
    /**
     * Validate processor configuration
     */
    virtual bool validate_config(const ProcessorConfig& config) const = 0;
};

/**
 * Processor filter interface for conditional processing
 */
class ProcessorFilter {
public:
    virtual ~ProcessorFilter() = default;
    
    /**
     * Check if a span should be processed
     */
    virtual bool should_process_span(const nlohmann::json& span_data) const = 0;
    
    /**
     * Check if a trace should be processed
     */
    virtual bool should_process_trace(const nlohmann::json& trace_data) const = 0;
};

/**
 * Filtered processor that applies filters before processing
 */
class FilteredProcessor : public TracingProcessor {
private:
    std::unique_ptr<TracingProcessor> processor_;
    std::vector<std::unique_ptr<ProcessorFilter>> filters_;
    
public:
    FilteredProcessor(
        std::unique_ptr<TracingProcessor> processor,
        std::vector<std::unique_ptr<ProcessorFilter>> filters
    ) : processor_(std::move(processor)), filters_(std::move(filters)) {}
    
    void process_span(const nlohmann::json& span_data) override {
        for (const auto& filter : filters_) {
            if (!filter->should_process_span(span_data)) {
                return;
            }
        }
        processor_->process_span(span_data);
    }
    
    void process_trace(const nlohmann::json& trace_data) override {
        for (const auto& filter : filters_) {
            if (!filter->should_process_trace(trace_data)) {
                return;
            }
        }
        processor_->process_trace(trace_data);
    }
    
    void process_spans_batch(const std::vector<nlohmann::json>& spans_data) override {
        std::vector<nlohmann::json> filtered_spans;
        for (const auto& span_data : spans_data) {
            bool should_process = true;
            for (const auto& filter : filters_) {
                if (!filter->should_process_span(span_data)) {
                    should_process = false;
                    break;
                }
            }
            if (should_process) {
                filtered_spans.push_back(span_data);
            }
        }
        if (!filtered_spans.empty()) {
            processor_->process_spans_batch(filtered_spans);
        }
    }
    
    void process_traces_batch(const std::vector<nlohmann::json>& traces_data) override {
        std::vector<nlohmann::json> filtered_traces;
        for (const auto& trace_data : traces_data) {
            bool should_process = true;
            for (const auto& filter : filters_) {
                if (!filter->should_process_trace(trace_data)) {
                    should_process = false;
                    break;
                }
            }
            if (should_process) {
                filtered_traces.push_back(trace_data);
            }
        }
        if (!filtered_traces.empty()) {
            processor_->process_traces_batch(filtered_traces);
        }
    }
    
    void flush() override { processor_->flush(); }
    void shutdown() override { processor_->shutdown(); }
    nlohmann::json get_config() const override { return processor_->get_config(); }
    nlohmann::json get_status() const override { return processor_->get_status(); }
};

/**
 * Composite processor that sends data to multiple processors
 */
class CompositeProcessor : public TracingProcessor {
private:
    std::vector<std::unique_ptr<TracingProcessor>> processors_;
    
public:
    explicit CompositeProcessor(std::vector<std::unique_ptr<TracingProcessor>> processors)
        : processors_(std::move(processors)) {}
    
    void process_span(const nlohmann::json& span_data) override {
        for (auto& processor : processors_) {
            try {
                processor->process_span(span_data);
            } catch (const std::exception& e) {
                // Log error but continue with other processors
                // This prevents one failing processor from stopping others
            }
        }
    }
    
    void process_trace(const nlohmann::json& trace_data) override {
        for (auto& processor : processors_) {
            try {
                processor->process_trace(trace_data);
            } catch (const std::exception& e) {
                // Log error but continue with other processors
            }
        }
    }
    
    void process_spans_batch(const std::vector<nlohmann::json>& spans_data) override {
        for (auto& processor : processors_) {
            try {
                processor->process_spans_batch(spans_data);
            } catch (const std::exception& e) {
                // Log error but continue with other processors
            }
        }
    }
    
    void process_traces_batch(const std::vector<nlohmann::json>& traces_data) override {
        for (auto& processor : processors_) {
            try {
                processor->process_traces_batch(traces_data);
            } catch (const std::exception& e) {
                // Log error but continue with other processors
            }
        }
    }
    
    void flush() override {
        for (auto& processor : processors_) {
            try {
                processor->flush();
            } catch (const std::exception& e) {
                // Log error but continue
            }
        }
    }
    
    void shutdown() override {
        for (auto& processor : processors_) {
            try {
                processor->shutdown();
            } catch (const std::exception& e) {
                // Log error but continue
            }
        }
    }
    
    nlohmann::json get_config() const override {
        nlohmann::json config = nlohmann::json::array();
        for (const auto& processor : processors_) {
            config.push_back(processor->get_config());
        }
        return config;
    }
    
    nlohmann::json get_status() const override {
        nlohmann::json status = nlohmann::json::array();
        for (const auto& processor : processors_) {
            status.push_back(processor->get_status());
        }
        return status;
    }
    
    /**
     * Add a processor to the composite
     */
    void add_processor(std::unique_ptr<TracingProcessor> processor) {
        processors_.push_back(std::move(processor));
    }
    
    /**
     * Get the number of processors
     */
    size_t get_processor_count() const {
        return processors_.size();
    }
};

/**
 * Async processor that queues operations for background processing
 */
class AsyncProcessor : public TracingProcessor {
private:
    std::unique_ptr<TracingProcessor> processor_;
    // Implementation would include threading and queueing
    // This is a simplified interface
    
public:
    explicit AsyncProcessor(std::unique_ptr<TracingProcessor> processor)
        : processor_(std::move(processor)) {}
    
    void process_span(const nlohmann::json& span_data) override {
        // Queue for async processing
        processor_->process_span(span_data);
    }
    
    void process_trace(const nlohmann::json& trace_data) override {
        // Queue for async processing
        processor_->process_trace(trace_data);
    }
    
    void flush() override { processor_->flush(); }
    void shutdown() override { processor_->shutdown(); }
    nlohmann::json get_config() const override { return processor_->get_config(); }
    nlohmann::json get_status() const override { return processor_->get_status(); }
};

/**
 * No-op processor that does nothing (for testing or disabled tracing)
 */
class NoOpProcessor : public TracingProcessor {
public:
    void process_span(const nlohmann::json& span_data) override {}
    void process_trace(const nlohmann::json& trace_data) override {}
    void flush() override {}
    void shutdown() override {}
    
    nlohmann::json get_config() const override {
        return nlohmann::json{{"type", "no-op"}};
    }
    
    nlohmann::json get_status() const override {
        return nlohmann::json{{"status", "inactive"}};
    }
};

/**
 * Function-based processor for simple custom logic
 */
class FunctionProcessor : public TracingProcessor {
private:
    std::function<void(const nlohmann::json&)> span_handler_;
    std::function<void(const nlohmann::json&)> trace_handler_;
    std::function<void()> flush_handler_;
    std::function<void()> shutdown_handler_;
    
public:
    FunctionProcessor(
        std::function<void(const nlohmann::json&)> span_handler,
        std::function<void(const nlohmann::json&)> trace_handler = nullptr,
        std::function<void()> flush_handler = nullptr,
        std::function<void()> shutdown_handler = nullptr
    ) : span_handler_(span_handler),
        trace_handler_(trace_handler),
        flush_handler_(flush_handler),
        shutdown_handler_(shutdown_handler) {}
    
    void process_span(const nlohmann::json& span_data) override {
        if (span_handler_) {
            span_handler_(span_data);
        }
    }
    
    void process_trace(const nlohmann::json& trace_data) override {
        if (trace_handler_) {
            trace_handler_(trace_data);
        }
    }
    
    void flush() override {
        if (flush_handler_) {
            flush_handler_();
        }
    }
    
    void shutdown() override {
        if (shutdown_handler_) {
            shutdown_handler_();
        }
    }
    
    nlohmann::json get_config() const override {
        return nlohmann::json{{"type", "function"}};
    }
    
    nlohmann::json get_status() const override {
        return nlohmann::json{{"status", "active"}};
    }
};

} // namespace tracing
} // namespace openai_agents