#include <gtest/gtest.h>

#include <snail/common/thread.hpp>

#include <chrono>
#include <condition_variable>
#include <thread>

#include <snail/common/thread_pool.hpp>

using namespace snail::common;

using namespace std::string_view_literals;

TEST(ThreadPool, SubmitSome)
{
    std::atomic<bool> end_task_1      = false;
    std::atomic<bool> task_1_started  = false;
    std::atomic<bool> task_1_finished = false;

    std::atomic<bool> end_task_2      = false;
    std::atomic<bool> task_2_started  = false;
    std::atomic<bool> task_2_finished = false;

    std::atomic<bool> end_task_3      = false;
    std::atomic<bool> task_3_started  = false;
    std::atomic<bool> task_3_finished = false;

    std::atomic<bool> end_task_4      = false;
    std::atomic<bool> task_4_started  = false;
    std::atomic<bool> task_4_finished = false;

    {
        thread_pool pool(2);

        // submit first task: should start right away
        pool.submit(
            [&end_task_1, &task_1_started, &task_1_finished]()
            {
                task_1_started = true;
                while(!end_task_1)
                    ;
                task_1_finished = true;
            });

        while(!task_1_started)
            ;
        EXPECT_TRUE(task_1_started);
        EXPECT_FALSE(task_1_finished);
        EXPECT_FALSE(task_2_started);
        EXPECT_FALSE(task_2_finished);
        EXPECT_FALSE(task_3_started);
        EXPECT_FALSE(task_3_finished);
        EXPECT_FALSE(task_4_started);
        EXPECT_FALSE(task_4_finished);

        // submit second task: should start right away, too
        pool.submit(
            [&end_task_2, &task_2_started, &task_2_finished]()
            {
                task_2_started = true;
                while(!end_task_2)
                    ;
                task_2_finished = true;
            });

        while(!task_2_started)
            ;
        EXPECT_TRUE(task_1_started);
        EXPECT_FALSE(task_1_finished);
        EXPECT_TRUE(task_2_started);
        EXPECT_FALSE(task_2_finished);
        EXPECT_FALSE(task_3_started);
        EXPECT_FALSE(task_3_finished);
        EXPECT_FALSE(task_4_started);
        EXPECT_FALSE(task_4_finished);

        // submit third task: should not start yet
        pool.submit(
            [&end_task_3, &task_3_started, &task_3_finished]()
            {
                task_3_started = true;
                while(!end_task_3)
                    ;
                task_3_finished = true;
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EXPECT_TRUE(task_1_started);
        EXPECT_FALSE(task_1_finished);
        EXPECT_TRUE(task_2_started);
        EXPECT_FALSE(task_2_finished);
        EXPECT_FALSE(task_3_started);
        EXPECT_FALSE(task_3_finished);
        EXPECT_FALSE(task_4_started);
        EXPECT_FALSE(task_4_finished);

        // end the first task. This should allow the third task to start
        end_task_1 = true;

        while(!task_3_started)
            ;

        EXPECT_TRUE(task_1_started);
        EXPECT_TRUE(task_1_finished);
        EXPECT_TRUE(task_2_started);
        EXPECT_FALSE(task_2_finished);
        EXPECT_TRUE(task_3_started);
        EXPECT_FALSE(task_3_finished);
        EXPECT_FALSE(task_4_started);
        EXPECT_FALSE(task_4_finished);

        // end the second task
        end_task_2 = true;
        while(!task_2_finished)
            ;

        EXPECT_TRUE(task_1_started);
        EXPECT_TRUE(task_1_finished);
        EXPECT_TRUE(task_2_started);
        EXPECT_TRUE(task_2_finished);
        EXPECT_TRUE(task_3_started);
        EXPECT_FALSE(task_3_finished);
        EXPECT_FALSE(task_4_started);
        EXPECT_FALSE(task_4_finished);

        // end the third task and stop the pool, which should join all threads
        end_task_3 = true;

        pool.stop();

        EXPECT_TRUE(task_1_started);
        EXPECT_TRUE(task_1_finished);
        EXPECT_TRUE(task_2_started);
        EXPECT_TRUE(task_2_finished);
        EXPECT_TRUE(task_3_started);
        EXPECT_TRUE(task_3_finished);
        EXPECT_FALSE(task_4_started);
        EXPECT_FALSE(task_4_finished);

        // Submit after stopped: will simply be ignored
        pool.submit(
            [&end_task_4, &task_4_started, &task_4_finished]()
            {
                task_4_started = true;
                while(!end_task_4)
                    ;
                task_4_finished = true;
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EXPECT_TRUE(task_1_started);
        EXPECT_TRUE(task_1_finished);
        EXPECT_TRUE(task_2_started);
        EXPECT_TRUE(task_2_finished);
        EXPECT_TRUE(task_3_started);
        EXPECT_TRUE(task_3_finished);
        EXPECT_FALSE(task_4_started);
        EXPECT_FALSE(task_4_finished);
    }

    EXPECT_TRUE(task_1_started);
    EXPECT_TRUE(task_1_finished);
    EXPECT_TRUE(task_2_started);
    EXPECT_TRUE(task_2_finished);
    EXPECT_TRUE(task_3_started);
    EXPECT_TRUE(task_3_finished);
    EXPECT_FALSE(task_4_started);
    EXPECT_FALSE(task_4_finished);
}
