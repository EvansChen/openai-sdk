#include "_coro.h"
#include <thread>
#include <chrono>

namespace openai_agents {
namespace util {

std::future<void> delay(int milliseconds) {
    return std::async(std::launch::async, [milliseconds]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    });
}

} // namespace util
} // namespace openai_agents