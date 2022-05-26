// coroexample_asyncscope.h                                           -*-C++-*-
#ifndef INCLUDED_COROEXAMPLE_ASYNCSCOPE
#define INCLUDED_COROEXAMPLE_ASYNCSCOPE

#include <coroutine>
#include <atomic>
#include <cassert>

#include <coroexample/coroexample_generalhelper.h>
#include <coroexample/coroexample_helper.h>

////////////////////////////////////////
// async_scope
//
// Used to launch new tasks and then later wait until all tasks
// have completed.

struct async_scope {
  private:
    struct detached_task {
        struct promise_type {
            async_scope& scope;

            promise_type(async_scope& scope, auto&) noexcept : scope(scope) {}

            detached_task get_return_object() noexcept { return {}; }

            std::suspend_never initial_suspend() noexcept {
                scope.add_ref();
                return {};
            }

            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                void
                await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    async_scope& s = h.promise().scope;
                    h.destroy();
                    s.notify_task_finished();
                }
                void await_resume() noexcept {}
            };

            final_awaiter final_suspend() noexcept { return {}; }

            void return_void() noexcept {}

            [[noreturn]] void unhandled_exception() noexcept {
                std::terminate();
            }
        };
    };

    template <typename A>
    detached_task spawn_detached_impl(A a) {
        co_await std::forward<A>(a);
    }

    void add_ref() noexcept {
        ref_count.fetch_add(ref_increment, std::memory_order_relaxed);
    }

    void notify_task_finished() noexcept {
        std::size_t oldValue = ref_count.load(std::memory_order_acquire);
        assert(oldValue >= ref_increment);

        if (oldValue > (joiner_flag + ref_increment)) {
            oldValue =
                ref_count.fetch_sub(ref_increment, std::memory_order_acq_rel);
        }

        if (oldValue == (joiner_flag + ref_increment)) {
            // last ref and there is a joining coroutine -> resume the
            // coroutien
            joiner.resume();
        }
    }

    struct join_awaiter {
        async_scope& scope;

        bool await_ready() {
            return scope.ref_count.load(std::memory_order_acquire) == 0;
        }

        bool await_suspend(std::coroutine_handle<> h) noexcept {
            scope.joiner         = h;
            std::size_t oldValue = scope.ref_count.fetch_add(
                joiner_flag, std::memory_order_acq_rel);
            return (oldValue != 0);
        }

        void await_resume() noexcept {}
    };

    static constexpr std::size_t joiner_flag   = 1;
    static constexpr std::size_t ref_increment = 2;

    std::atomic<std::size_t> ref_count{0};
    std::coroutine_handle<>  joiner;

  public:
    template <typename A>
    requires decay_copyable<A> && awaitable<std::decay_t<A>>
    void spawn_detached(A&& a) { spawn_detached_impl(std::forward<A>(a)); }

    [[nodiscard]] join_awaiter join_async() noexcept {
        return join_awaiter{*this};
    }
};

#endif
