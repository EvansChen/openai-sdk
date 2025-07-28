#pragma once

namespace openai_agents {

/**
 * Details about input tokens
 */
struct InputTokensDetails {
    int cached_tokens = 0;
    
    InputTokensDetails(int cached = 0) : cached_tokens(cached) {}
};

/**
 * Details about output tokens
 */
struct OutputTokensDetails {
    int reasoning_tokens = 0;
    
    OutputTokensDetails(int reasoning = 0) : reasoning_tokens(reasoning) {}
};

/**
 * Token usage statistics for LLM API calls
 */
class Usage {
public:
    Usage();
    
    /**
     * Add usage statistics from another Usage object
     * @param other The other Usage object to add
     */
    void add(const Usage& other);
    
    // Getters
    int get_requests() const { return requests_; }
    int get_input_tokens() const { return input_tokens_; }
    int get_output_tokens() const { return output_tokens_; }
    int get_total_tokens() const { return total_tokens_; }
    const InputTokensDetails& get_input_tokens_details() const { return input_tokens_details_; }
    const OutputTokensDetails& get_output_tokens_details() const { return output_tokens_details_; }
    
    // Setters
    void set_requests(int requests) { requests_ = requests; }
    void set_input_tokens(int tokens) { input_tokens_ = tokens; }
    void set_output_tokens(int tokens) { output_tokens_ = tokens; }
    void set_total_tokens(int tokens) { total_tokens_ = tokens; }
    void set_input_tokens_details(const InputTokensDetails& details) { input_tokens_details_ = details; }
    void set_output_tokens_details(const OutputTokensDetails& details) { output_tokens_details_ = details; }

private:
    int requests_ = 0;                              ///< Total requests made to the LLM API
    int input_tokens_ = 0;                          ///< Total input tokens sent, across all requests
    InputTokensDetails input_tokens_details_;       ///< Details about the input tokens
    int output_tokens_ = 0;                         ///< Total output tokens received, across all requests
    OutputTokensDetails output_tokens_details_;     ///< Details about the output tokens
    int total_tokens_ = 0;                          ///< Total tokens sent and received, across all requests
};

} // namespace openai_agents