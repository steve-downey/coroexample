// coroexample_manualeventloop.h                                      -*-C++-*-
#ifndef INCLUDED_COROEXAMPLE_MANUALEVENTLOOP
#define INCLUDED_COROEXAMPLE_MANUALEVENTLOOP

#include <condition_variable>
#include <coroutine>
#include <mutex>

/////////////////////////////////////////////////
// manual_event_loop
//
// A simple scheduler context with an intrusive list.
//
// Uses mutex/condition_variable for synchronisation and supports
// multiple work threads running tasks.

struct manual_event_loop {
  private:
    struct queue_item {
        queue_item*             next;
        std::coroutine_handle<> coro;
    };

    std::mutex              mut;
    std::condition_variable cv;
    queue_item*             head{nullptr};
    queue_item*             tail{nullptr};

    void enqueue(queue_item* item) noexcept {
        std::lock_guard lock{mut};
        item->next = nullptr;
        if (head == nullptr) {
            head = item;
        } else {
            tail->next = item;
        }
        tail = item;
        cv.notify_one();
    }

    queue_item* pop_item() noexcept {
        queue_item* front = head;
        if (head != nullptr) {
            head = front->next;
            if (head == nullptr) {
                tail = nullptr;
            }
        }
        return front;
    }

    struct schedule_awaitable {
        manual_event_loop* loop;
        queue_item         item;

        explicit schedule_awaitable(manual_event_loop& loop) noexcept
            : loop(&loop) {}

        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> coro) noexcept {
            item.coro = coro;
            loop->enqueue(&item);
        }
        void await_resume() noexcept {}
    };

  public:
    schedule_awaitable schedule() noexcept {
        return schedule_awaitable{*this};
    }

    void run(std::stop_token st) noexcept {
        std::stop_callback cb{st, [&]() noexcept {
                                  std::lock_guard lock{mut};
                                  cv.notify_all();
                              }};

        std::unique_lock lock{mut};
        while (true) {
            cv.wait(lock, [&]() noexcept {
                return head != nullptr || st.stop_requested();
            });
            if (st.stop_requested()) {
                return;
            }

            queue_item* item = pop_item();

            lock.unlock();
            item->coro.resume();
            lock.lock();
        }
    }
};

#endif
