#pragma once

#include <string>
#include <memory>
#include <any>

namespace openai_agents {

// Base result class
class Result {
public:
    virtual ~Result() = default;
    virtual bool is_success() const = 0;
    virtual std::any get_data() const = 0;
    virtual std::string get_error_message() const = 0;
};

// Specific result implementations
class SuccessResult : public Result {
public:
    SuccessResult(const std::any& data) : data_(data) {}
    
    bool is_success() const override { return true; }
    std::any get_data() const override { return data_; }
    std::string get_error_message() const override { return ""; }
    
private:
    std::any data_;
};

class ErrorResult : public Result {
public:
    ErrorResult(const std::string& error) : error_(error) {}
    
    bool is_success() const override { return false; }
    std::any get_data() const override { return std::any{}; }
    std::string get_error_message() const override { return error_; }
    
private:
    std::string error_;
};

} // namespace openai_agents