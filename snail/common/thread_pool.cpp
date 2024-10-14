#include <snail/common/thread_pool.hpp>

#include <format>

#include <snail/common/thread.hpp>

using namespace snail::common;

thread_pool::thread_pool(std::size_t max_size) :
    max_size_(max_size),
    stopped_(false)
{}

thread_pool::~thread_pool()
{
    stop();
}

void thread_pool::submit(task_type task)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);

        if(stopped_) return; // do not submit jobs to thread pool that was stopped

        task_queue_.push(std::move(task));
    }

    condition_variable_.notify_one();

    if(workers_.size() < max_size_)
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // If the task has not been picked up, try to start a new thread.
        if(!task_queue_.empty()) spawn_thread();
    }
}

void thread_pool::stop()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stopped_  = true;
        max_size_ = 0;
    }
    condition_variable_.notify_all();
    for(auto& worker : workers_)
    {
        worker.join();
    }
    workers_.clear();
}

void thread_pool::spawn_thread()
{
    const auto id = workers_.size() + 1;

    workers_.emplace_back(
        [this, id]
        {
            set_thread_name(std::format("Worker {}", id));
            while(true)
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(mutex_);

                    // TODO: only wait for a given time and shut down this
                    // thread if we have not received any work.
                    condition_variable_.wait(lock, [this]
                                             { return stopped_ || !task_queue_.empty(); });
                    if(stopped_ && task_queue_.empty()) return;

                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }

                task();
            }
        });
}
