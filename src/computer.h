#pragma once

/**
 * Computer interaction and environment
 */

namespace openai_agents {

// Environment types
enum class Environment {
    DESKTOP,
    MOBILE,
    WEB,
    VIRTUAL
};

// Button interface
class Button {
public:
    virtual ~Button() = default;
    virtual void click() = 0;
    virtual std::string get_text() const = 0;
    virtual bool is_enabled() const = 0;
};

// Computer interface (synchronous)
class Computer {
public:
    virtual ~Computer() = default;
    virtual void screenshot(const std::string& filename) = 0;
    virtual void click(int x, int y) = 0;
    virtual void type(const std::string& text) = 0;
    virtual void scroll(int delta_x, int delta_y) = 0;
};

// Async computer interface
class AsyncComputer {
public:
    virtual ~AsyncComputer() = default;
    // Async methods would return futures/coroutines in a real implementation
    virtual std::future<void> screenshot(const std::string& filename) = 0;
    virtual std::future<void> click(int x, int y) = 0;
    virtual std::future<void> type(const std::string& text) = 0;
};

} // namespace openai_agents