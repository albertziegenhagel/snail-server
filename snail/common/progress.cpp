
#include <snail/common/progress.hpp>

#include <algorithm>
#include <cmath>

using namespace snail::common;

progress_listener::progress_listener(double resolution) :
    resolution_(resolution)
{}

double progress_listener::resolution() const
{
    return resolution_;
}

void cancellation_token::cancel()
{
    cancel_ = true;
}

bool cancellation_token::is_canceled() const
{
    return cancel_.load();
}

progress_reporter::progress_reporter(const progress_listener*        listener,
                                     work_type                       total_work,
                                     std::string_view                title,
                                     std::optional<std::string_view> message) :
    listener_(listener),
    total_work_(total_work),
    step_work_(((listener != nullptr) ? listener->resolution() : 1.0) * total_work_),
    current_work_(0),
    next_report_step_(1),
    next_report_work_(static_cast<work_type>(std::ceil(step_work_ * next_report_step_)))
{
    if(listener_ != nullptr)
    {
        listener_->start(title, message);
        listener_->report(0.0, message);
    }
}

void progress_reporter::progress(work_type                       work,
                                 std::optional<std::string_view> message)
{
    current_work_ += work;
    if(current_work_ < next_report_work_) return;

    const auto current_progress = double(current_work_) / total_work_;
    if(listener_ != nullptr) listener_->report(current_progress, message);

    const auto current_step = static_cast<unsigned int>(double(current_work_) / step_work_);

    next_report_step_ = std::max(next_report_step_ + 1u, current_step + 1u);

    next_report_work_ = static_cast<work_type>(std::ceil(step_work_ * next_report_step_));

    if(current_work_ < total_work_ && next_report_work_ > total_work_)
    {
        // Make sure we always report at 100%
        // This does also fix issues with rounding around 100%.
        next_report_work_ = total_work_;
        --next_report_step_; // Since the value we will report at next is not a real
                             // step, we should not count it as one.
                             // The easiest thing is to simply decrement the step counter
                             // here.
    }
}

void progress_reporter::finish(std::optional<std::string_view> message)
{
    if(listener_ == nullptr) return;
    if(current_work_ < total_work_) listener_->report(1.0, message);
    listener_->finish(message);
}
