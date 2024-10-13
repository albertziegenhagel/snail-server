#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace snail::common {

class thread_pool
{
public:
    using task_type = std::function<void()>;

    explicit thread_pool(std::size_t max_size);
    ~thread_pool();

    void submit(task_type task);

    void stop();

private:
    std::size_t              max_size_;
    std::vector<std::thread> workers_;

    std::queue<task_type> task_queue_;

    std::mutex              mutex_;
    std::condition_variable condition_variable_;

    bool stopped_;

    void spawn_thread();
};

} // namespace snail::common
