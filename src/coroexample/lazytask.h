// coroexample_lazytask.h                                             -*-C++-*-
#ifndef INCLUDED_COROEXAMPLE_LAZYTASK
#define INCLUDED_COROEXAMPLE_LAZYTASK

#include <thread>

#include <coroexample/helper.h>

////////////////////////////////////////////////
// Helper for improving allocation elision for composed operations.
//
// Instead of doing something like:
//
//   task<T> h(int arg);
//   scope.spawn_detached(h(42));
//
// which will generally separately allocate the h() coroutine as well
// as the internal detached_task coroutine, if we write:
//
//   scope.spawn_detached(lazy_task{[] { return h(42); }});
//
// then this defers calling the `h()` coroutine function to the evaluation
// of `operator co_await()` in the `detached_task` coroutine, which then
// permits the compiler to elide the allocation of `h()` coroutine and
// combine its storage into the `detached_task` coroutine state, meaning
// that we now have one allocation per spawned task instead of two.

template <typename F>
struct lazy_task {
    F func;
    using task_t    = std::invoke_result_t<F&>;
    using awaiter_t = awaiter_type_t<task_t>;

    struct awaiter {
        task_t    task;
        awaiter_t inner;

        explicit awaiter(F& func) noexcept(
            std::is_nothrow_invocable_v<F&>&& noexcept(
                get_awaiter(static_cast<task_t&&>(task))))
            : task(func()), inner(get_awaiter(static_cast<task_t&&>(task))) {}

        decltype(auto) await_ready() noexcept(noexcept(inner.await_ready())) {
            return inner.await_ready();
        }
        decltype(auto)
        await_suspend(auto h) noexcept(noexcept(inner.await_suspend(h))) {
            return inner.await_suspend(h);
        }
        decltype(auto)
        await_resume() noexcept(noexcept(inner.await_resume())) {
            return inner.await_resume();
        }
    };

    awaiter operator co_await() noexcept(
        std::is_nothrow_constructible_v<awaiter, F&>) {
        return awaiter{func};
    }
};

template <typename F>
lazy_task(F) -> lazy_task<F>;

#endif
