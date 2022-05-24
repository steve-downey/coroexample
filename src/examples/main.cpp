#include <coroexample/coroexample.h>

#include <iostream>

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
