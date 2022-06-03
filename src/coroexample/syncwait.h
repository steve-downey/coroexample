// coroexample_syncwait.h                                             -*-C++-*-
#ifndef INCLUDED_COROEXAMPLE_SYNCWAIT
#define INCLUDED_COROEXAMPLE_SYNCWAIT

#include <variant>
#include <exception>
#include <semaphore>
#include <coroutine>

#include <coroexample/generalhelper.h>
#include <coroexample/helper.h>

////////////////////////////////////////////////////////////////
// sync_wait()

template <typename Task>
await_result_t<Task> sync_wait(Task&& t) {
    struct _void {};
    using return_type  = await_result_t<Task>;
    using storage_type = std::add_pointer_t<
        std::conditional_t<std::is_void_v<return_type>, _void, return_type>>;
    using result_type =
        std::variant<std::monostate, storage_type, std::exception_ptr>;

    struct _sync_task {
        struct promise_type {
            std::binary_semaphore sem{0};
            result_type           result;

            _sync_task get_return_object() noexcept {
                return _sync_task{
                    std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                void
                await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    // Now that coroutine has suspended we can signal the
                    // semaphore, unblocking the waiting thread. The other
                    // thread will then destroy this coroutine (which is safe
                    // because it is suspended).
                    h.promise().sem.release();
                }
                void await_resume() noexcept {}
            };

            std::suspend_always initial_suspend() noexcept { return {}; }

            final_awaiter final_suspend() noexcept { return {}; }

            using non_void_return_type = std::
                conditional_t<std::is_void_v<return_type>, _void, return_type>;

            final_awaiter yield_value(non_void_return_type&& x)
            requires(!std::is_void_v<return_type>)
            {
                // Note that we just store the address here and then suspend
                // and unblock the waiting thread which then copies/moves the
                // result from this address directly to the return-value of
                // sync_wait(). This avoids having to make an extra
                // intermediate copy of the result value.
                result.template emplace<1>(std::addressof(x));
                return {};
            }

            void return_void() noexcept { result.template emplace<1>(); }

            void unhandled_exception() noexcept {
                result.template emplace<2>(std::current_exception());
            }
        };

        using handle_t = std::coroutine_handle<promise_type>;
        handle_t coro;

        explicit _sync_task(handle_t h) noexcept : coro(h) {}
        _sync_task(_sync_task&& o) noexcept
            : coro(std::exchange(o.coro, {})) {}
        ~_sync_task() {
            if (coro)
                coro.destroy();
        }

        // The force-inline here is required to get the _sync_task coroutine
        // elided. Otherwise the compiler doesn't know that this function
        // hasn't modified 'coro' member variable and so can't deduce that it's
        // always destroyed before sync_wait() returns.
        FORCE_INLINE return_type run() {
            coro.resume();
            coro.promise().sem.acquire();

            auto& result = coro.promise().result;
            if (result.index() == 2) {
                std::rethrow_exception(std::get<2>(std::move(result)));
            }

            assert(result.index() == 1);

            if constexpr (!std::is_void_v<return_type>) {
                return static_cast<return_type&&>(*std::get<1>(result));
            }
        }
    };

    return [&]() -> _sync_task {
        if constexpr (std::is_void_v<return_type>) {
            co_await static_cast<Task&&>(t);
        } else {
            // use co_yield instead of co_return so we suspend while the
            // potentially temporary result of co_await is still alive.
            co_yield co_await static_cast<Task&&>(t);
        }
    }()
                        .run();
}

#endif
