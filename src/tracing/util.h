#pragma once

/**
 * Tracing Utilities for OpenAI Agents Framework
 * 
 * This module provides utility functions for working with trace data.
 */

#include "span_data.h"
#include "spans.h"
#include "traces.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <random>

namespace openai_agents {
namespace tracing {
namespace util {

/**
 * ID generation utilities
 */
namespace id_gen {

/**
 * Generate a random hex string of specified length
 */
std::string random_hex(size_t length);

/**
 * Generate a trace ID in the standard format
 */
std::string trace_id();

/**
 * Generate a span ID in the standard format
 */
std::string span_id();

/**
 * Generate a UUID v4
 */
std::string uuid();

} // namespace id_gen

/**
 * Time utilities
 */
namespace time {

/**
 * Get current time as ISO 8601 string
 */
std::string current_iso();

/**
 * Parse ISO 8601 time string to time_point
 */
std::optional<std::chrono::system_clock::time_point> parse_iso(const std::string& iso_string);

/**
 * Convert time_point to ISO 8601 string
 */
std::string to_iso(const std::chrono::system_clock::time_point& time_point);

/**
 * Calculate duration between two ISO time strings
 */
std::optional<std::chrono::milliseconds> duration_between(
    const std::string& start_iso, 
    const std::string& end_iso
);

/**
 * Get timestamp as milliseconds since epoch
 */
int64_t timestamp_ms();

} // namespace time

/**
 * JSON utilities for trace data
 */
namespace json {

/**
 * Safely convert std::any to JSON
 */
nlohmann::json any_to_json(const std::any& value);

/**
 * Convert std::unordered_map<std::string, std::any> to JSON
 */
nlohmann::json any_map_to_json(const std::unordered_map<std::string, std::any>& map);

/**
 * Validate span data JSON
 */
bool is_valid_span(const nlohmann::json& span_json);

/**
 * Validate trace data JSON
 */
bool is_valid_trace(const nlohmann::json& trace_json);

/**
 * Extract span type from span data JSON
 */
std::optional<std::string> get_span_type(const nlohmann::json& span_json);

/**
 * Extract trace ID from span or trace JSON
 */
std::optional<std::string> get_trace_id(const nlohmann::json& json);

/**
 * Extract span ID from span JSON
 */
std::optional<std::string> get_span_id(const nlohmann::json& span_json);

/**
 * Pretty print JSON with proper formatting
 */
std::string pretty_print(const nlohmann::json& json, int indent = 2);

} // namespace json

/**
 * Span analysis utilities
 */
namespace analysis {

/**
 * Calculate span tree depth
 */
size_t calculate_span_tree_depth(const std::vector<const AnySpan*>& spans);

/**
 * Build span hierarchy from flat list
 */
struct SpanNode {
    const AnySpan* span;
    std::vector<std::unique_ptr<SpanNode>> children;
    
    explicit SpanNode(const AnySpan* span) : span(span) {}
};

std::vector<std::unique_ptr<SpanNode>> build_span_tree(const std::vector<const AnySpan*>& spans);

/**
 * Find root spans (spans with no parent)
 */
std::vector<const AnySpan*> find_root_spans(const std::vector<const AnySpan*>& spans);

/**
 * Find child spans of a given parent
 */
std::vector<const AnySpan*> find_child_spans(
    const std::vector<const AnySpan*>& spans, 
    const std::string& parent_span_id
);

/**
 * Calculate total span count by type
 */
std::unordered_map<std::string, size_t> count_spans_by_type(const std::vector<const AnySpan*>& spans);

/**
 * Find spans with errors
 */
std::vector<const AnySpan*> find_error_spans(const std::vector<const AnySpan*>& spans);

/**
 * Calculate trace statistics
 */
struct TraceStatistics {
    size_t total_spans = 0;
    size_t error_spans = 0;
    size_t max_depth = 0;
    std::chrono::milliseconds total_duration{0};
    std::unordered_map<std::string, size_t> span_type_counts;
    std::optional<std::chrono::system_clock::time_point> start_time;
    std::optional<std::chrono::system_clock::time_point> end_time;
};

TraceStatistics calculate_trace_stats(const std::vector<const AnySpan*>& spans);

} // namespace analysis

/**
 * Export utilities
 */
namespace export_util {

/**
 * Export format options
 */
enum class ExportFormat {
    JSON,
    CSV,
    TEXT,
    JAEGER_JSON,
    ZIPKIN_JSON
};

/**
 * Export spans to different formats
 */
std::string export_spans(
    const std::vector<const AnySpan*>& spans,
    ExportFormat format = ExportFormat::JSON
);

/**
 * Export trace to different formats
 */
std::string export_trace(
    const Trace& trace,
    ExportFormat format = ExportFormat::JSON
);

/**
 * Convert to Jaeger-compatible format
 */
nlohmann::json to_jaeger_format(const std::vector<const AnySpan*>& spans);

/**
 * Convert to Zipkin-compatible format
 */
nlohmann::json to_zipkin_format(const std::vector<const AnySpan*>& spans);

/**
 * Convert to CSV format
 */
std::string to_csv(const std::vector<const AnySpan*>& spans);

/**
 * Convert to human-readable text format
 */
std::string to_text(const std::vector<const AnySpan*>& spans);

} // namespace export_util

/**
 * Filtering utilities
 */
namespace filter {

/**
 * Filter predicate type
 */
using SpanPredicate = std::function<bool(const AnySpan&)>;

/**
 * Common filter predicates
 */
SpanPredicate by_type(const std::string& span_type);
SpanPredicate by_name(const std::string& name);
SpanPredicate has_error();
SpanPredicate duration_greater_than(std::chrono::milliseconds min_duration);
SpanPredicate duration_less_than(std::chrono::milliseconds max_duration);
SpanPredicate time_range(
    const std::chrono::system_clock::time_point& start,
    const std::chrono::system_clock::time_point& end
);

/**
 * Combine predicates with logical operations
 */
SpanPredicate operator&&(const SpanPredicate& left, const SpanPredicate& right);
SpanPredicate operator||(const SpanPredicate& left, const SpanPredicate& right);
SpanPredicate operator!(const SpanPredicate& predicate);

/**
 * Apply filter to span collection
 */
std::vector<const AnySpan*> apply_filter(
    const std::vector<const AnySpan*>& spans,
    const SpanPredicate& predicate
);

} // namespace filter

/**
 * Debugging utilities
 */
namespace debug {

/**
 * Print span tree to console
 */
void print_span_tree(const std::vector<const AnySpan*>& spans);

/**
 * Print trace summary
 */
void print_trace_summary(const Trace& trace);

/**
 * Print span details
 */
void print_span_details(const AnySpan& span);

/**
 * Validate trace consistency
 */
struct ValidationResult {
    bool is_valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

ValidationResult validate_trace_consistency(const std::vector<const AnySpan*>& spans);

/**
 * Find orphaned spans (spans with missing parents)
 */
std::vector<const AnySpan*> find_orphaned_spans(const std::vector<const AnySpan*>& spans);

/**
 * Check for circular dependencies in span hierarchy
 */
bool has_circular_dependencies(const std::vector<const AnySpan*>& spans);

} // namespace debug

} // namespace util
} // namespace tracing
} // namespace openai_agents