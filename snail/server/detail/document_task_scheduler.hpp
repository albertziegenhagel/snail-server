#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#include <snail/common/thread_pool.hpp>

#include <snail/server/detail/document_id.hpp>

namespace snail::server::detail {

enum class document_access_type
{
    read_only,
    write
};

// Schedules tasks for documents.
// The idea is that we can always execute tasks that operate on different
// documents concurrently.
// Tasks for a specific document need to be executed in order if they alter
// the state of the document (i.e. if they are "writing"), but tasks that only
// read document can be performed concurrently as well.
class document_task_scheduler
{
public:
    using task_type = std::function<void()>;

    explicit document_task_scheduler(common::thread_pool& pool);
    ~document_task_scheduler();

    void schedule(document_id id, document_access_type access_type, task_type task);

    void shutdown();

private:
    common::thread_pool* pool_;

    std::mutex              mutex_;
    std::condition_variable condition_variable_;

    bool shutdown_;

    std::thread thread_;

    struct document_task
    {
        document_access_type access_type_;
        task_type            task_;
    };

    struct document_queue
    {
        bool        running_write_ = false;
        std::size_t running_reads_ = 0;

        std::queue<document_task> queue_;
    };

    std::unordered_map<document_id, document_queue> document_queues_;

    std::queue<document_id> ready_documents_;

    bool has_ready_task() const;
    bool has_any_task() const;

    bool is_queue_ready(const document_queue& queue) const;
};

} // namespace snail::server::detail
