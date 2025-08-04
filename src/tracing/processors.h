#pragma once

/**
 * Built-in Tracing Processors for OpenAI Agents Framework
 * 
 * This module provides common processor implementations for trace data.
 */

#include "processor_interface.h"
#include "../logger.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace openai_agents {
namespace tracing {

/**
 * Console processor that outputs trace data to stdout/stderr
 */
class ConsoleProcessor : public TracingProcessor {
private:
    bool use_stderr_;
    bool pretty_print_;
    std::mutex output_mutex_;
    
public:
    explicit ConsoleProcessor(bool use_stderr = false, bool pretty_print = true)
        : use_stderr_(use_stderr), pretty_print_(pretty_print) {}
    
    void process_span(const nlohmann::json& span_data) override;
    void process_trace(const nlohmann::json& trace_data) override;
    
    nlohmann::json get_config() const override {
        return nlohmann::json{
            {"type", "console"},
            {"use_stderr", use_stderr_},
            {"pretty_print", pretty_print_}
        };
    }
    
    nlohmann::json get_status() const override {
        return nlohmann::json{{"status", "active"}};
    }
};

/**
 * File processor that writes trace data to files
 */
class FileProcessor : public TracingProcessor {
private:
    std::string file_path_;
    std::ofstream file_stream_;
    std::mutex file_mutex_;
    bool append_mode_;
    
public:
    explicit FileProcessor(const std::string& file_path, bool append_mode = true);
    ~FileProcessor();
    
    void process_span(const nlohmann::json& span_data) override;
    void process_trace(const nlohmann::json& trace_data) override;
    void flush() override;
    void shutdown() override;
    
    nlohmann::json get_config() const override {
        return nlohmann::json{
            {"type", "file"},
            {"file_path", file_path_},
            {"append_mode", append_mode_}
        };
    }
    
    nlohmann::json get_status() const override {
        return nlohmann::json{
            {"status", file_stream_.is_open() ? "active" : "error"},
            {"file_path", file_path_}
        };
    }
};

/**
 * HTTP processor that sends trace data to a remote endpoint
 */
class HttpProcessor : public TracingProcessor {
private:
    std::string endpoint_url_;
    std::string api_key_;
    std::unordered_map<std::string, std::string> headers_;
    size_t batch_size_;
    std::chrono::milliseconds batch_timeout_;
    
    // Batch processing
    std::queue<nlohmann::json> span_queue_;
    std::queue<nlohmann::json> trace_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread worker_thread_;
    std::atomic<bool> should_stop_{false};
    
    void worker_loop();
    void send_batch();
    bool send_http_request(const nlohmann::json& data);
    
public:
    HttpProcessor(
        const std::string& endpoint_url,
        const std::string& api_key = "",
        const std::unordered_map<std::string, std::string>& headers = {},
        size_t batch_size = 100,
        std::chrono::milliseconds batch_timeout = std::chrono::milliseconds(5000)
    );
    
    ~HttpProcessor();
    
    void process_span(const nlohmann::json& span_data) override;
    void process_trace(const nlohmann::json& trace_data) override;
    void flush() override;
    void shutdown() override;
    
    nlohmann::json get_config() const override {
        return nlohmann::json{
            {"type", "http"},
            {"endpoint_url", endpoint_url_},
            {"batch_size", batch_size_},
            {"batch_timeout_ms", batch_timeout_.count()}
        };
    }
    
    nlohmann::json get_status() const override {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return nlohmann::json{
            {"status", should_stop_ ? "stopped" : "active"},
            {"queued_spans", span_queue_.size()},
            {"queued_traces", trace_queue_.size()}
        };
    }
};

/**
 * Memory processor that stores trace data in memory for testing
 */
class MemoryProcessor : public TracingProcessor {
private:
    std::vector<nlohmann::json> spans_;
    std::vector<nlohmann::json> traces_;
    mutable std::mutex data_mutex_;
    size_t max_items_;
    
public:
    explicit MemoryProcessor(size_t max_items = 10000) : max_items_(max_items) {}
    
    void process_span(const nlohmann::json& span_data) override;
    void process_trace(const nlohmann::json& trace_data) override;
    
    std::vector<nlohmann::json> get_spans() const;
    std::vector<nlohmann::json> get_traces() const;
    void clear();
    
    nlohmann::json get_config() const override {
        return nlohmann::json{
            {"type", "memory"},
            {"max_items", max_items_}
        };
    }
    
    nlohmann::json get_status() const override {
        std::lock_guard<std::mutex> lock(data_mutex_);
        return nlohmann::json{
            {"status", "active"},
            {"stored_spans", spans_.size()},
            {"stored_traces", traces_.size()}
        };
    }
};

/**
 * Default processor factory implementation
 */
class DefaultProcessorFactory : public ProcessorFactory {
public:
    std::unique_ptr<TracingProcessor> create_processor(const ProcessorConfig& config) override;
    std::vector<std::string> get_supported_types() const override;
    bool validate_config(const ProcessorConfig& config) const override;
};

} // namespace tracing
} // namespace openai_agents