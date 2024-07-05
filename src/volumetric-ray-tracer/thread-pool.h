#include "include/definitions.h"
#include <condition_variable>
#include <functional>
#include <queue>

// BUG: Something is causing a deadlock in here...
struct thread_pool_t
{
    std::condition_variable cond;
    std::mutex mutex;
    std::condition_variable done;
    std::mutex done_mutex;
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> threads;
    bool stopped = false;
    u64 threads_idle = 0;
    const u64 thread_count;

    const std::function<void()> thrd_func = [this] () {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex);
                cond.wait(lock, [this](){ return !tasks.empty() || stopped; });
                {
                    std::unique_lock<std::mutex> lock(done_mutex);
                    threads_idle--;
                }
                if (stopped) return;
                task = tasks.front();
                tasks.pop();
            }
            task();
            {
                std::unique_lock<std::mutex> lock(done_mutex);
                threads_idle++;
                if (tasks.empty() && threads_idle == thread_count)
                {
                    done.notify_one();
                }
            }
        }
    };

    thread_pool_t(u64 thread_count = std::thread::hardware_concurrency()) : thread_count(thread_count)
    {
        for (u64 i = 0; i < thread_count; ++i)
            threads.emplace_back(thrd_func);
    }
    void enqueue(std::function<void()> task)
    {
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->tasks.emplace(task);
        }
        //this->cond.notify_one();
    }
    void start_work() {
        this->threads_idle = thread_count;
        this->cond.notify_all();
    }
    ~thread_pool_t()
    {
        {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->stopped = true;
        }
        this->cond.notify_all();
        for (std::thread &t : this->threads)
            t.join();
    }
};
