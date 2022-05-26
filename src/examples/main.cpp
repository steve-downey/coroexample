#include <coroexample/coroexample.h>

static task<int> f(int i) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1ms);

    co_return i;
}

static task<int> g(int i, manual_event_loop& loop) {
    co_await loop.schedule();
    int x = co_await f(i);
    co_return x + 1;
}

static task<void> h(int i, manual_event_loop& loop) {
    int  x  = co_await g(i, loop);
    auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
    std::printf(
        "[%u] %i -> %i (on %i)\n", (unsigned int)ts, i, x, (int)::gettid());
}

static task<void> nested_scopes(int x, manual_event_loop& loop) {
    co_await loop.schedule();

    async_scope scope;
    try {
        for (int i = 0; i < 10; ++i) {
            scope.spawn_detached(h(i, loop));
        }
    } catch (...) {
        std::printf("failure!\n");
    }

    co_await scope.join_async();

    std::printf("nested %i done\n", x);
    std::fflush(stdout);
}

int main() {
    manual_event_loop loop;

    std::jthread thd{[&](std::stop_token st) { loop.run(st); }};
    std::jthread thd2{[&](std::stop_token st) { loop.run(st); }};

    std::printf("starting example\n");

    {
        async_scope scope;
        scope_guard join_on_exit{[&] { sync_wait(scope.join_async()); }};

        for (int i = 0; i < 10; ++i) {
            // Use lazy_task here so that h() coroutine allocation is elided
            // and incorporated into spawn_detached() allocation.
            scope.spawn_detached(lazy_task{[i, &loop] { return h(i, loop); }});
        }
    }

    std::printf("starting nested_scopes example\n");

    {
        async_scope scope;
        scope_guard join_on_exit{[&] { sync_wait(scope.join_async()); }};

        for (int i = 0; i < 10; ++i) {
            scope.spawn_detached(
                lazy_task{[i, &loop] { return nested_scopes(i, loop); }});
        }
    }

    return 0;
}
