#include <gtest/gtest.h>

#include <snail/common/progress.hpp>

#include <array>
#include <cmath>

using namespace snail::common;

using namespace std::string_view_literals;

namespace {

struct collecting_progress_listener : public progress_listener
{
    using progress_listener::progress_listener;

    virtual void start() const override
    {
        started = true;
    }

    virtual void report(double progress) const override
    {
        reported.push_back(std::round(progress * 1000.0) / 10.0); // percent rounded to one digit after the point
    }

    virtual void finish() const override
    {
        finished = true;
    }

    mutable bool                started  = false;
    mutable bool                finished = false;
    mutable std::vector<double> reported;
};

} // namespace

TEST(Progress, UnitWork100)
{
    const collecting_progress_listener listener(0.1);
    progress_reporter                  reporter(&listener, 100);

    for(progress_reporter::work_type work = 0; work < 100; ++work)
    {
        reporter.progress(1);
    }

    reporter.finish();

    EXPECT_TRUE(listener.started);
    EXPECT_EQ(listener.reported,
              (std::vector<double>{0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}));
    EXPECT_TRUE(listener.finished);
}

TEST(Progress, UnitWork123)
{
    const collecting_progress_listener listener(0.1);
    progress_reporter                  reporter(&listener, 123);

    for(progress_reporter::work_type work = 0; work < 123; ++work)
    {
        reporter.progress(1);
    }

    reporter.finish();

    EXPECT_TRUE(listener.started);
    EXPECT_EQ(listener.reported,
              (std::vector<double>{0.0, 10.6, 20.3, 30.1, 40.7, 50.4, 60.2, 70.7, 80.5, 90.2, 100.0}));
    EXPECT_TRUE(listener.finished);
}

TEST(Progress, UnitWork7)
{
    const collecting_progress_listener listener(0.1);
    progress_reporter                  reporter(&listener, 7);

    for(progress_reporter::work_type work = 0; work < 7; ++work)
    {
        reporter.progress(1);
    }

    reporter.finish();

    EXPECT_TRUE(listener.started);
    EXPECT_EQ(listener.reported,
              (std::vector<double>{0.0, 14.3, 28.6, 42.9, 57.1, 71.4, 85.7, 100.0}));
    EXPECT_TRUE(listener.finished);
}

TEST(Progress, UnitWorkOddReport100)
{
    const collecting_progress_listener listener(0.13);
    progress_reporter                  reporter(&listener, 100);

    for(progress_reporter::work_type work = 0; work < 100; ++work)
    {
        reporter.progress(1);
    }

    reporter.finish();

    EXPECT_TRUE(listener.started);
    EXPECT_EQ(listener.reported,
              (std::vector<double>{0.0, 13.0, 26.0, 39.0, 52.0, 65.0, 78.0, 91.0, 100.0}));
    EXPECT_TRUE(listener.finished);
}

TEST(Progress, UnitWorkOddOverWorkReport100)
{
    const collecting_progress_listener listener(0.13);
    progress_reporter                  reporter(&listener, 100);

    for(progress_reporter::work_type work = 0; work < 130; ++work)
    {
        reporter.progress(1);
    }

    reporter.finish();

    EXPECT_TRUE(listener.started);
    EXPECT_EQ(listener.reported,
              (std::vector<double>{0.0, 13.0, 26.0, 39.0, 52.0, 65.0, 78.0, 91.0, 100.0, 104.0, 117.0, 130.0}));
    EXPECT_TRUE(listener.finished);
}

TEST(Progress, JumpWork100)
{
    const collecting_progress_listener listener(0.1);
    progress_reporter                  reporter(&listener, 100);

    // progress = 0
    reporter.progress(15); // progress = 15
    reporter.progress(25); // progress = 40
    reporter.progress(29); // progress = 69
    reporter.progress(1);  // progress = 70
    reporter.progress(1);  // progress = 71
    reporter.progress(8);  // progress = 79
    reporter.progress(6);  // progress = 85

    reporter.finish();

    EXPECT_TRUE(listener.started);
    EXPECT_EQ(listener.reported,
              (std::vector<double>{0.0, 15.0, 40.0, 69.0, 70.0, 85.0, 100.0}));
    EXPECT_TRUE(listener.finished);
}

TEST(Progress, OverWork100)
{
    const collecting_progress_listener listener(0.1);
    progress_reporter                  reporter(&listener, 100);

    for(progress_reporter::work_type work = 0; work < 200; ++work)
    {
        reporter.progress(1);
    }

    reporter.finish();

    EXPECT_TRUE(listener.started);
    EXPECT_EQ(listener.reported,
              (std::vector<double>{0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0,
                                   110.0, 120.0, 130.0, 140.0, 150.0, 160.0, 170.0, 180.0, 190.0, 200.0}));
    EXPECT_TRUE(listener.finished);
}

TEST(Progress, WorkRoundIssue)
{
    const collecting_progress_listener listener(0.1);
    const progress_reporter::work_type total = 5373952;
    progress_reporter                  reporter(&listener, total);

    const progress_reporter::work_type first = 4836568;
    reporter.progress(first);
    const auto remaining = total - first;
    reporter.progress(remaining);

    reporter.finish();

    EXPECT_TRUE(listener.started);
    EXPECT_EQ(listener.reported,
              (std::vector<double>{0.0, 90.0, 100.0}));
    EXPECT_TRUE(listener.finished);
}

TEST(Progress, NoListener)
{
    progress_reporter reporter(nullptr, 100);

    // just test that it does not crash?

    reporter.progress(20);
    reporter.progress(50);
    reporter.progress(100);

    reporter.finish();
}

TEST(Cancellation, Cancel)
{
    cancellation_token token;

    EXPECT_FALSE(token.is_canceled());

    token.cancel();

    EXPECT_TRUE(token.is_canceled());
}
