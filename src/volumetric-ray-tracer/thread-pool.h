#include "include/definitions.h"
#include <condition_variable>
#include <functional>
#include <queue>

struct thread_pool_t
{
    std::condition_variable cond;
    std::mutex queue_mutex;
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> threads;
    bool stopped = false;

    const std::function<void()> thrd_func = [this] () {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cond.wait(lock, [this](){ return !tasks.empty() || stopped; });
                if (stopped && tasks.empty()) return;
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
    };

    thread_pool_t(u64 thread_count = std::thread::hardware_concurrency())
    {
        for (u64 i = 0; i < thread_count; ++i)
            threads.emplace_back(thrd_func);
    }
    void enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->tasks.emplace(task);
        }
        this->cond.notify_one();
    }
    ~thread_pool_t()
    {
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->stopped = true;
        }
        this->cond.notify_all();
        for (std::thread &t : this->threads)
            t.join();
    }
};
