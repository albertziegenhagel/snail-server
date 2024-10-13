#include <snail/server/detail/document_task_scheduler.hpp>

#include <cassert>

#include <snail/common/thread.hpp>

using namespace snail::server::detail;

document_task_scheduler::document_task_scheduler(common::thread_pool& pool) :
    pool_(&pool),
    shutdown_(false)
{
    thread_ = std::thread(
        [this]()
        {
            common::set_thread_name("Document Task Scheduler");
            while(true)
            {
                std::unique_lock<std::mutex> lock(mutex_);

                // wait for a task become ready
                condition_variable_.wait(lock, [this]
                                         { return (shutdown_ && !has_any_task()) || has_ready_task(); });
                if(shutdown_ && !has_any_task()) return;
                assert(has_ready_task());

                // extract the next task for the next ready document
                const auto ready_document_id = ready_documents_.front();
                ready_documents_.pop();

                auto& ready_queue = document_queues_.at(ready_document_id);
                assert(is_queue_ready(ready_queue));

                auto ready_task = std::move(ready_queue.queue_.front());
                ready_queue.queue_.pop();

                // update the document queues state
                if(ready_task.access_type_ == document_access_type::read_only)
                {
                    ++ready_queue.running_reads_;
                }
                else
                {
                    assert(!ready_queue.running_write_);
                    ready_queue.running_write_ = true;
                }

                // If the queue for the document is still ready, put the document
                // back onto the ready list (but enqueue in the end, to make sure
                // other documents get a chance to run their tasks as well)
                const auto is_still_ready = is_queue_ready(ready_queue);
                if(is_still_ready)
                {
                    ready_documents_.push(ready_document_id);
                }

                // determine whether this task is the reason that the queue
                // is not ready anymore (i.e. usually because it is either
                // a writing task, or it is the last read-only task and the
                // next one is writing).
                // const auto is_blocking_the_queue = !is_still_ready &&
                //                                    !ready_queue.queue_.empty();

                // const auto _is_blocking_the_queue = ready_queue.queue_.empty() ||
                //                                     ready_queue.running_write_ ||
                //                                     ((ready_queue.queue_.front().access_type_ == document_access_type::write) && ready_queue.running_reads_ > 0);

                lock.unlock();

                pool_->submit(
                    [this,
                     task = std::move(ready_task),
                     &ready_queue,
                     was_still_ready = is_still_ready,
                     ready_document_id]()
                    {
                        // Execute the actual task
                        task.task_();

                        {
                            std::unique_lock<std::mutex> lock(mutex_);

                            // Update the tasks queue state
                            if(task.access_type_ == document_access_type::read_only)
                            {
                                assert(ready_queue.running_reads_ > 0);
                                --ready_queue.running_reads_;
                            }
                            else
                            {
                                assert(ready_queue.running_write_);
                                ready_queue.running_write_ = false;
                            }

                            // If the queue was not ready anymore (e.g. because this
                            // task blocked the queue or the queue was just empty)
                            // but the queue is not empty anymore, we can continue with
                            // the queue.
                            if(!was_still_ready && !ready_queue.queue_.empty())
                            {
                                assert(is_queue_ready(ready_queue));
                                ready_documents_.push(ready_document_id);
                            }
                        }

                        condition_variable_.notify_one();
                    });
            }
        });
}

document_task_scheduler::~document_task_scheduler()
{
    shutdown();
}

void document_task_scheduler::schedule(document_id id, document_access_type access_type, task_type task)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);

        auto iter = document_queues_.find(id);
        if(iter == document_queues_.end())
        {
            document_queues_[id].queue_.emplace(
                document_task{
                    .access_type_ = access_type,
                    .task_        = std::move(task)});
            ready_documents_.push(id);
        }
        else
        {
            iter->second.queue_.emplace(
                document_task{
                    .access_type_ = access_type,
                    .task_        = std::move(task)});

            if(iter->second.queue_.size() == 1 &&
               !iter->second.running_write_ &&
               iter->second.running_reads_ == 0)
            {
                ready_documents_.push(id);
            }
        }
    }
    condition_variable_.notify_one();
}

void document_task_scheduler::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if(shutdown_) return; // has already been shut down
        shutdown_ = true;
    }
    condition_variable_.notify_all();
    thread_.join();
}

bool document_task_scheduler::has_ready_task() const
{
    // // NOTE: only call with mutex locked!
    return !ready_documents_.empty();
}

bool document_task_scheduler::has_any_task() const
{
    // NOTE: only call with mutex locked!
    for(const auto& [id, queue] : document_queues_)
    {
        if(!queue.queue_.empty()) return true;
    }
    return false;
}

bool document_task_scheduler::is_queue_ready(const document_queue& queue) const
{
    // NOTE: only call with mutex locked!
    if(queue.queue_.empty()) return false;
    if(queue.running_write_) return false;
    const auto& next_task = queue.queue_.front();
    return (next_task.access_type_ == document_access_type::read_only) || queue.running_reads_ == 0;
}
