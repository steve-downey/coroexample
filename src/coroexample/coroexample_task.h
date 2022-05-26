// coroexample_task.h                                                 -*-C++-*-
#ifndef INCLUDED_COROEXAMPLE_TASK
#define INCLUDED_COROEXAMPLE_TASK

#include <coroutine>
#include <variant>
#include <exception>
#include <utility>
#include <cassert>

///////////////////////////////////////////////////
// task<T> - basic async task type

template <typename T>
struct task;

template <typename T>
struct task_promise {
    task<T> get_return_object() noexcept;

    std::suspend_always initial_suspend() noexcept { return {}; }

    struct final_awaiter {
        bool await_ready() noexcept { return false; }
        std::coroutine_handle<>
        await_suspend(std::coroutine_handle<task_promise> h) noexcept {
            return h.promise().continuation;
        }
        [[noreturn]] void await_resume() noexcept { std::terminate(); }
    };

    final_awaiter final_suspend() noexcept { return {}; }

    template <typename U>
    requires std::convertible_to<U, T>
    void
    return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>) {
        result.template emplace<1>(std::forward<U>(value));
    }

    void unhandled_exception() noexcept {
        result.template emplace<2>(std::current_exception());
    }

    std::coroutine_handle<>                             continuation;
    std::variant<std::monostate, T, std::exception_ptr> result;
};

template <>
struct task_promise<void> {
    task<void> get_return_object() noexcept;

    std::suspend_always initial_suspend() noexcept { return {}; }

    struct final_awaiter {
        bool await_ready() noexcept { return false; }
        std::coroutine_handle<>
        await_suspend(std::coroutine_handle<task_promise> h) noexcept {
            return h.promise().continuation;
        }
        [[noreturn]] void await_resume() noexcept { std::terminate(); }
    };

    final_awaiter final_suspend() noexcept { return {}; }

    void return_void() noexcept { result.emplace<1>(); }

    void unhandled_exception() noexcept {
        result.emplace<2>(std::current_exception());
    }

    struct empty {};

    std::coroutine_handle<>                                 continuation;
    std::variant<std::monostate, empty, std::exception_ptr> result;
};

template <typename T>
struct [[nodiscard]] task {
  private:
    using handle_t = std::coroutine_handle<task_promise<T>>;
    handle_t coro;

    struct awaiter {
        handle_t coro;
        bool     await_ready() noexcept { return false; }

        handle_t await_suspend(std::coroutine_handle<> h) noexcept {
            coro.promise().continuation = h;
            return coro;
        }

        T await_resume() {
            if (coro.promise().result.index() == 2) {
                std::rethrow_exception(
                    std::get<2>(std::move(coro.promise().result)));
            }

            assert(coro.promise().result.index() == 1);

            if constexpr (!std::is_void_v<T>) {
                return std::get<1>(std::move(coro.promise().result));
            }
        }
    };

    friend struct task_promise<T>;

    explicit task(handle_t h) noexcept : coro(h) {}

  public:
    using promise_type = task_promise<T>;

    task(task&& other) noexcept : coro(std::exchange(other.coro, {})) {}

    ~task() {
        if (coro)
            coro.destroy();
    }

    awaiter operator co_await() && { return awaiter{coro}; }
};

template <typename T>
task<T> task_promise<T>::get_return_object() noexcept {
    return task<T>{
        std::coroutine_handle<task_promise<T>>::from_promise(*this)};
}

task<void> task_promise<void>::get_return_object() noexcept {
    return task<void>{
        std::coroutine_handle<task_promise<void>>::from_promise(*this)};
}

#endif
