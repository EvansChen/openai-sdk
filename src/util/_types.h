#pragma once

/**
 * Utility Types for OpenAI Agents Framework
 * 
 * This module provides common type utilities and type aliases
 * used throughout the OpenAI Agents framework.
 */

#include <future>
#include <type_traits>
#include <variant>
#include <functional>

namespace openai_agents {
namespace util {

/**
 * Template variable to check if a type is awaitable (has a future-like interface)
 */
template<typename T>
struct is_awaitable : std::false_type {};

template<typename T>
struct is_awaitable<std::future<T>> : std::true_type {};

template<typename T>
struct is_awaitable<std::shared_future<T>> : std::true_type {};

template<typename T>
inline constexpr bool is_awaitable_v = is_awaitable<T>::value;

/**
 * Type trait to extract the value type from an awaitable type
 */
template<typename T>
struct awaitable_value_type {
    using type = T;
};

template<typename T>
struct awaitable_value_type<std::future<T>> {
    using type = T;
};

template<typename T>
struct awaitable_value_type<std::shared_future<T>> {
    using type = T;
};

template<typename T>
using awaitable_value_type_t = typename awaitable_value_type<T>::type;

/**
 * Type alias for values that may or may not be awaitable
 * 
 * This is equivalent to Python's MaybeAwaitable = Union[Awaitable[T], T]
 * In C++, we use std::variant to represent the union of synchronous value
 * and asynchronous future.
 * 
 * @tparam T The value type
 */
template<typename T>
using MaybeAwaitable = std::variant<T, std::future<T>>;

/**
 * Helper function to check if a MaybeAwaitable contains an immediate value
 */
template<typename T>
bool is_immediate(const MaybeAwaitable<T>& maybe_awaitable) {
    return std::holds_alternative<T>(maybe_awaitable);
}

/**
 * Helper function to check if a MaybeAwaitable contains a future
 */
template<typename T>
bool is_future(const MaybeAwaitable<T>& maybe_awaitable) {
    return std::holds_alternative<std::future<T>>(maybe_awaitable);
}

/**
 * Helper function to get the immediate value from a MaybeAwaitable
 * Throws std::bad_variant_access if it doesn't contain an immediate value
 */
template<typename T>
const T& get_immediate(const MaybeAwaitable<T>& maybe_awaitable) {
    return std::get<T>(maybe_awaitable);
}

/**
 * Helper function to get the future from a MaybeAwaitable
 * Throws std::bad_variant_access if it doesn't contain a future
 */
template<typename T>
std::future<T>& get_future(MaybeAwaitable<T>& maybe_awaitable) {
    return std::get<std::future<T>>(maybe_awaitable);
}

/**
 * Helper function to resolve a MaybeAwaitable to its value
 * If it contains an immediate value, returns it directly
 * If it contains a future, waits for it and returns the result
 */
template<typename T>
T resolve(MaybeAwaitable<T>&& maybe_awaitable) {
    if (is_immediate(maybe_awaitable)) {
        return std::move(std::get<T>(maybe_awaitable));
    } else {
        return std::get<std::future<T>>(maybe_awaitable).get();
    }
}

/**
 * Helper function to create a MaybeAwaitable from an immediate value
 */
template<typename T>
MaybeAwaitable<T> make_immediate(T value) {
    return MaybeAwaitable<T>{std::move(value)};
}

/**
 * Helper function to create a MaybeAwaitable from a future
 */
template<typename T>
MaybeAwaitable<T> make_future(std::future<T> future) {
    return MaybeAwaitable<T>{std::move(future)};
}

/**
 * Helper function to create a MaybeAwaitable from an async operation
 */
template<typename F, typename... Args>
auto make_async(F&& func, Args&&... args) -> MaybeAwaitable<std::invoke_result_t<F, Args...>> {
    using ReturnType = std::invoke_result_t<F, Args...>;
    auto future = std::async(std::launch::async, std::forward<F>(func), std::forward<Args>(args)...);
    return make_future<ReturnType>(std::move(future));
}

/**
 * Function pointer type alias for convenience
 */
template<typename Signature>
using FunctionPtr = std::function<Signature>;

/**
 * Type alias for callable objects
 */
template<typename Signature>
using Callable = std::function<Signature>;

/**
 * Type trait to check if a type is callable
 */
template<typename T>
struct is_callable : std::false_type {};

template<typename R, typename... Args>
struct is_callable<std::function<R(Args...)>> : std::true_type {};

template<typename R, typename... Args>
struct is_callable<R(*)(Args...)> : std::true_type {};

template<typename T>
inline constexpr bool is_callable_v = is_callable<T>::value;

/**
 * Type erased callable wrapper
 */
class AnyCallable {
public:
    template<typename F>
    AnyCallable(F&& func) : func_(std::forward<F>(func)) {}

    template<typename... Args>
    auto operator()(Args&&... args) -> decltype(func_(std::forward<Args>(args)...)) {
        return func_(std::forward<Args>(args)...);
    }

private:
    std::function<void()> func_;
};

/**
 * Optional reference wrapper - similar to std::optional but for references
 */
template<typename T>
class optional_ref {
public:
    optional_ref() : ptr_(nullptr) {}
    optional_ref(T& ref) : ptr_(&ref) {}
    optional_ref(const optional_ref&) = default;
    optional_ref& operator=(const optional_ref&) = default;

    bool has_value() const { return ptr_ != nullptr; }
    explicit operator bool() const { return has_value(); }

    T& value() const {
        if (!has_value()) {
            throw std::runtime_error("optional_ref has no value");
        }
        return *ptr_;
    }

    T& operator*() const { return value(); }
    T* operator->() const { return &value(); }

    void reset() { ptr_ = nullptr; }

private:
    T* ptr_;
};

/**
 * Helper function to create optional_ref
 */
template<typename T>
optional_ref<T> make_optional_ref(T& ref) {
    return optional_ref<T>(ref);
}

/**
 * Type alias for common numeric types
 */
using Int = int;
using Long = long long;
using Float = float;
using Double = double;
using Size = std::size_t;

/**
 * Type alias for string types
 */
using String = std::string;
using StringView = std::string_view;

} // namespace util
} // namespace openai_agents