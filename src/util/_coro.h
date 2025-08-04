#pragma once

/**
 * Coroutine Utilities for OpenAI Agents Framework
 * 
 * This module provides coroutine-related utilities and helper functions
 * for asynchronous operations in the OpenAI Agents framework.
 * 
 * In C++, we use std::future and std::async for asynchronous operations
 * instead of Python's asyncio coroutines.
 */

#include <future>
#include <functional>
#include <memory>

namespace openai_agents {
namespace util {

/**
 * No-operation coroutine equivalent
 * 
 * This is the C++ equivalent of Python's async def noop_coroutine() -> None: pass
 * Returns a future that immediately resolves to void.
 */
inline std::future<void> noop_coroutine() {
    std::promise<void> promise;
    promise.set_value();
    return promise.get_future();
}

/**
 * Create a future that resolves immediately with the given value
 * 
 * @tparam T The type of the value
 * @param value The value to resolve with
 * @return A future that immediately resolves to the given value
 */
template<typename T>
std::future<T> immediate_future(T value) {
    std::promise<T> promise;
    promise.set_value(std::move(value));
    return promise.get_future();
}

/**
 * Create a future that resolves immediately with void
 * 
 * @return A future that immediately resolves to void
 */
inline std::future<void> immediate_void_future() {
    return noop_coroutine();
}

/**
 * Create a future that fails immediately with the given exception
 * 
 * @tparam T The type of the future
 * @param exception The exception to fail with
 * @return A future that immediately fails with the given exception
 */
template<typename T>
std::future<T> failed_future(std::exception_ptr exception) {
    std::promise<T> promise;
    promise.set_exception(exception);
    return promise.get_future();
}

/**
 * Create a future that fails immediately with the given exception
 * 
 * @tparam T The type of the future
 * @tparam E The exception type
 * @param exception The exception to fail with
 * @return A future that immediately fails with the given exception
 */
template<typename T, typename E>
std::future<T> failed_future(E exception) {
    return failed_future<T>(std::make_exception_ptr(exception));
}

/**
 * Utility to chain futures together
 * 
 * @tparam T The type of the first future
 * @tparam F The type of the continuation function
 * @param future The future to chain from
 * @param continuation The function to call when the future completes
 * @return A new future representing the chained operation
 */
template<typename T, typename F>
auto then(std::future<T> future, F&& continuation) -> std::future<std::invoke_result_t<F, T>> {
    using ReturnType = std::invoke_result_t<F, T>;
    
    return std::async(std::launch::async, [future = std::move(future), continuation = std::forward<F>(continuation)]() mutable -> ReturnType {
        T result = future.get();
        return continuation(std::move(result));
    });
}

/**
 * Utility to chain void futures together
 * 
 * @tparam F The type of the continuation function
 * @param future The future to chain from
 * @param continuation The function to call when the future completes
 * @return A new future representing the chained operation
 */
template<typename F>
auto then_void(std::future<void> future, F&& continuation) -> std::future<std::invoke_result_t<F>> {
    using ReturnType = std::invoke_result_t<F>;
    
    return std::async(std::launch::async, [future = std::move(future), continuation = std::forward<F>(continuation)]() mutable -> ReturnType {
        future.get(); // Wait for completion, ignore result
        return continuation();
    });
}

/**
 * Wait for all futures to complete
 * 
 * @tparam T The type of the futures
 * @param futures Vector of futures to wait for
 * @return A future that completes when all input futures complete
 */
template<typename T>
std::future<std::vector<T>> wait_all(std::vector<std::future<T>> futures) {
    return std::async(std::launch::async, [futures = std::move(futures)]() mutable -> std::vector<T> {
        std::vector<T> results;
        results.reserve(futures.size());
        
        for (auto& future : futures) {
            results.push_back(future.get());
        }
        
        return results;
    });
}

/**
 * Wait for all void futures to complete
 * 
 * @param futures Vector of void futures to wait for
 * @return A future that completes when all input futures complete
 */
inline std::future<void> wait_all_void(std::vector<std::future<void>> futures) {
    return std::async(std::launch::async, [futures = std::move(futures)]() mutable {
        for (auto& future : futures) {
            future.get();
        }
    });
}

/**
 * Wait for any future to complete (returns the first one to complete)
 * 
 * @tparam T The type of the futures
 * @param futures Vector of futures to wait for
 * @return A future containing the result of the first future to complete
 */
template<typename T>
std::future<T> wait_any(std::vector<std::future<T>> futures) {
    return std::async(std::launch::async, [futures = std::move(futures)]() mutable -> T {
        // Simple implementation - wait for all and return the first
        // A more sophisticated implementation would use platform-specific APIs
        // to wait for the first completion
        if (futures.empty()) {
            throw std::invalid_argument("Cannot wait for any of zero futures");
        }
        
        // For simplicity, just return the first future's result
        // In a real implementation, you'd want to use select/poll/epoll etc.
        return futures[0].get();
    });
}

/**
 * Create a future that completes after a delay
 * 
 * @param milliseconds The delay in milliseconds
 * @return A future that completes after the specified delay
 */
std::future<void> delay(int milliseconds);

/**
 * Create a future that times out after a specified duration
 * 
 * @tparam T The type of the future
 * @param future The future to add timeout to
 * @param timeout_ms Timeout in milliseconds
 * @return A future that either completes with the original result or times out
 */
template<typename T>
std::future<T> with_timeout(std::future<T> future, int timeout_ms) {
    return std::async(std::launch::async, [future = std::move(future), timeout_ms]() mutable -> T {
        auto status = future.wait_for(std::chrono::milliseconds(timeout_ms));
        if (status == std::future_status::timeout) {
            throw std::runtime_error("Future timed out after " + std::to_string(timeout_ms) + "ms");
        }
        return future.get();
    });
}

/**
 * Async function wrapper
 * 
 * Wraps a regular function to run asynchronously
 * 
 * @tparam F The function type
 * @tparam Args The argument types
 * @param func The function to run asynchronously
 * @param args The arguments to pass to the function
 * @return A future representing the asynchronous operation
 */
template<typename F, typename... Args>
auto async_call(F&& func, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    return std::async(std::launch::async, std::forward<F>(func), std::forward<Args>(args)...);
}

/**
 * Synchronous wrapper for async operations
 * 
 * Blocks until the future completes and returns the result
 * 
 * @tparam T The type of the future
 * @param future The future to wait for
 * @return The result of the future
 */
template<typename T>
T sync_await(std::future<T> future) {
    return future.get();
}

/**
 * Synchronous wrapper for void async operations
 * 
 * @param future The void future to wait for
 */
inline void sync_await_void(std::future<void> future) {
    future.get();
}

} // namespace util
} // namespace openai_agents