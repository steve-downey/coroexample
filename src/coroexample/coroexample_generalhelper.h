// coroutineexample_generalhelper.h                                  -*-C++-*-

#ifndef INCLUDED_COROEXAMPLE_GENERALHELPER
#define INCLUDED_COROEXAMPLE_GENERALHELPER

#include <concepts>
#include <utility>

///////////////////////////////////////////////////
// general helpers

#define FORCE_INLINE __attribute__((always_inline))

template <typename T>
concept decay_copyable = std::constructible_from<std::decay_t<T>, T>;

template <typename F>
struct scope_guard {
    F    func;
    bool cancelled{false};

    template <typename F2>
    requires std::constructible_from<F, F2>
    explicit scope_guard(F2&& f) noexcept(
        std::is_nothrow_constructible_v<F, F2>)
        : func(static_cast<F2>(f)) {}

    scope_guard(scope_guard&& g) noexcept requires
        std::is_nothrow_move_constructible_v<F>
        : func(std::move(g.func)),
          cancelled(std::exchange(g.cancelled, true)) {}

    ~scope_guard() { call_now(); }

    void cancel() noexcept { cancelled = true; }

    void call_now() noexcept {
        if (!cancelled) {
            cancelled = true;
            func();
        }
    }
};

template <typename F>
scope_guard(F) -> scope_guard<F>;

#endif
