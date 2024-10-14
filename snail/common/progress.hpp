#pragma once

#include <atomic>
#include <cstddef>

namespace snail::common {

class progress_listener
{
public:
    progress_listener(double resolution = 0.01);

    double resolution() const;

    virtual void start() const                 = 0;
    virtual void report(double progress) const = 0;
    virtual void finish() const                = 0;

private:
    double resolution_;
};

class cancellation_token
{
public:
    void cancel();
    bool is_canceled() const;

private:
    std::atomic<bool> cancel_ = false;
};

class progress_reporter
{
public:
    using work_type = std::size_t;

    progress_reporter(const progress_listener* listener,
                      work_type                total_work);

    void start();
    void progress(work_type work);
    void finish();

private:
    const progress_listener* listener_;

    const work_type total_work_;

    const double step_work_;

    work_type current_work_;

    unsigned int next_report_step_;
    work_type    next_report_work_;
};

} // namespace snail::common
