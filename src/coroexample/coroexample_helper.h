// coroexample_helper.h                                               -*-C++-*-
#ifndef INCLUDED_COROEXAMPLE_HELPER
#define INCLUDED_COROEXAMPLE_HELPER

#include <utility>

///////////////////////////////////////////////////
// coroutine helpers

// Concept that checks if a type is a valid "awaiter" type.
// i.e. has the await_ready/await_suspend/await_resume methods.
//
// Note that we can't check whether await_suspend() is valid here because
// we don't know what type of coroutine_handle<> to test it with.
// So we just check for await_ready/await_resume and assume if it has both
// of those then it will also have the await_suspend() method.
template <typename T>
concept awaiter = requires(T& x) {
                      (x.await_ready() ? (void)0 : (void)0);
                      x.await_resume();
                  };

template <typename T>
concept _member_co_await = requires(T&& x) {
                               {
                                   static_cast<T&&>(x).operator co_await()
                                   } -> awaiter;
                           };

template <typename T>
concept _free_co_await = requires(T&& x) {
                             {
                                 operator co_await(static_cast<T&&>(x))
                                 } -> awaiter;
                         };

template <typename T>
concept awaitable = _member_co_await<T> || _free_co_await<T> || awaiter<T>;

// get_awaiter(x) -> awaiter
//
// Helper function that tries to mimic what the compiler does in `co_await`
// expressions to obtain the awaiter for a given awaitable argument.
//
// It's not a perfect match, however, as we can't exactly match the overload
// resolution which combines both free-function overloads and member-function
// overloads of `operator co_await()` into a single overload-set.
//
// The `get_awaiter()` function will be an ambiguous call if a type has both
// a free-function `operator co_await()` and a member-function `operator
// co_await()` even if the compiler's overload resolution would not consider
// this to be ambiguous.
template <typename T>
    requires _member_co_await<T> decltype(auto)
get_awaiter(T&& x) noexcept(
    noexcept(static_cast<T&&>(x).operator co_await())) {
    return static_cast<T&&>(x).operator co_await();
}

template <typename T>
    requires _free_co_await<T> decltype(auto)
    get_awaiter(T&& x) noexcept(operator co_await(static_cast<T&&>(std::declval<T>()))) {
    return operator co_await(static_cast<T&&>(x));
}

template <typename T>
    requires awaiter<T> && (!_free_co_await<T> && !_member_co_await<T>)
T&& get_awaiter(T&& x) noexcept {
    return static_cast<T&&>(x);
}

template <typename T>
    requires awaitable<T>
using awaiter_type_t = decltype(get_awaiter(std::declval<T>()));

template <typename T>
    requires awaitable<T>
using await_result_t =
    decltype(std::declval<awaiter_type_t<T>&>().await_resume());

#endif
