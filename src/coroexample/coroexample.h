/////////////////////////////////////////////////////////////////////////
// This is a minimal example that shows the 4 main building-blocks needed to
// write concurrent/async coroutine code.
//
// 1. A coroutine type that lets users write their coroutine logic
//    and call and co_await other coroutines that they write.
//    This allows composition of async coroutine code.
//
//    This example defines a basic `task<T>` type that fulfills this purpose.
//
// 2. A mechanism for launching concurrent async operations and later
//    waiting for launched concurrent work to complete.
//
//    To be able to have multiple coroutines executing independently we need
//    some way to introduce concurrency. And to ensure that we are able to
//    safely compose concurrent operations and shut down cleanly, we need
//    some way to be able to wait for concurrent operations to complete so
//    that we can. e.g. ensure that the completion of those concurrent
//    operations "happens before" the destruction of resources used by those
//    concurrent operations.
//
//    This example defines a simple `async_scope` facility that lets you
//    spawn multiple coroutines that can run independently, keeping an atomic
//    reference count of the number of unfinished tasks that have been
//    launched. It also gives a way to asynchronously wait until all tasks have
//    completed.
//
// 3. A mechanism for blocking a thread until some async operation completes.
//
//    The main() function is a synchronous function and so if we launch some
//    async code we need some way to be able to block until that code completes
//    so that we don't return from main() until all concurrent work has been
//    joined.
//
//    This example defines a sync_wait() function that takes an awaitable which
//    it co_awaits and then blocks the current thread until the co_await
//    expression completes, returning the result of the co_await expression.
//
// 4. A mechanism for multiplexing multiple coroutines onto a set of worker
// threads.
//
//    One of the main reasons for writing asynchronous code is to allow a
//    thread to do something else while waiting for some operation to complete.
//    This requires some way to schedule/multiplex multiple coroutines onto a
//    smaller number of threads, typically using a queue and having an
//    event-loop run by each thread that allows it to do some work until that
//    work suspends and then pick up the next item in the queue and execute
//    that in the meantime to keep the thread busy.
//
//    This example provides a basic `manual_event_loop` implementation that
//    allows a coroutine to `co_await loop.schedule()` to suspend and enqueue
//    itself to the loop's queue, whereby a thread that is calling `loop.run()`
//    will eventually pick up that item and resume it.
//
//    In practice, such multiplexers often also support other kinds of
//    scheduling such as 'schedule when an I/O operation completes' or
//    'schedule when a time elapses'.
//
// These 4 components are essential to being able to write asynchronous
// coroutine code.
//
// Different coroutine library implementations may structure these facilities
// in different ways, sometimes combining these items into one abstraction.
// e.g. sometimes a multiplexer implementation might combine items 2, 3 and 4
// by providing a mechanism to launch a coroutine on that multiplexer and also
// wait for all launched work on that multiplexer.
//
// This example choses to separate them so that you can understand each
// component separately - each of the classes are relatively short (roughly 100
// lines) so should hopefully be relatively easy to study.
//
// However, keeping them separate also generally gives better flexibility with
// how to compose them into an application. e.g. see how we can reuse
// async_scope in the nested_scopes() example below.
//
// This example also defines a number of helper concepts/traits needed by some
// of the implementations:
// - `awaitable` concept
// - `awaiter` concept
// - `await_result_t<T>` type-trait
// - `awaiter_type_t<T>` type-trait
// - `get_awaiter(x) -> awaiter` helper function

// And some other helpers:
// - `lazy_task` - useful for improving coroutine allocation-elision
// - `scope_guard`
//
//
// Please feel free to use this code however you like - it is primarily
// intended for learning coroutine mechanisms rather than necessarily as
// production-quality code. However, attribution is appreciated if you do use
// it somewhere.
//
// By Lewis Baker <lewissbaker@gmail.com>
/////////////////////////////////////////////////////////////////////////

#include <coroexample/coroexample_generalhelper.h>
#include <coroexample/coroexample_helper.h>
#include <coroexample/coroexample_task.h>
#include <coroexample/coroexample_asyncscope.h>
#include <coroexample/coroexample_syncwait.h>
#include <coroexample/coroexample_manualeventloop.h>
#include <coroexample/coroexample_lazytask.h>
