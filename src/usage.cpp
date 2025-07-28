#include "usage.h"

namespace openai_agents {

Usage::Usage() 
    : requests_(0)
    , input_tokens_(0)
    , input_tokens_details_(0)
    , output_tokens_(0)
    , output_tokens_details_(0)
    , total_tokens_(0) {
}

void Usage::add(const Usage& other) {
    requests_ += other.requests_;
    input_tokens_ += other.input_tokens_;
    output_tokens_ += other.output_tokens_;
    total_tokens_ += other.total_tokens_;
    
    input_tokens_details_ = InputTokensDetails(
        input_tokens_details_.cached_tokens + other.input_tokens_details_.cached_tokens
    );
    
    output_tokens_details_ = OutputTokensDetails(
        output_tokens_details_.reasoning_tokens + other.output_tokens_details_.reasoning_tokens
    );
}

} // namespace openai_agents