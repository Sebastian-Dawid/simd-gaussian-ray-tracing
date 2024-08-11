#include "thread-pool.h"

thread_pool_t::thread_pool_t(u64 thread_count)
{
    for (u64 i = 0; i < thread_count; ++i)
        threads.emplace_back(thrd_func);
}

void thread_pool_t::enqueue(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        this->tasks.emplace(task);
    }
    this->cond.notify_one();
}

thread_pool_t::~thread_pool_t()
{
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        this->stopped = true;
    }
    this->cond.notify_all();
    for (std::thread &t : this->threads)
        t.join();
}
