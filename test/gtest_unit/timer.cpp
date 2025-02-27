#include "gtest/gtest.h"
#include <boost/thread.hpp>
#include "coroutine.h"
#include <vector>
#include <list>
#include <atomic>
#include <boost/timer.hpp>
using namespace co;

double second_duration(std::chrono::system_clock::time_point start)
{
    auto now = std::chrono::system_clock::now();
    auto duration = now - start;
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0;
}

TEST(Timer, OnTime1)
{
//    g_Scheduler.GetOptions().debug = dbg_timer;
    auto start = std::chrono::system_clock::now();
    int c = 1;
    co_timer_add(std::chrono::seconds(1), [&]{
            --c;
            EXPECT_LT(std::abs(1.0 - second_duration(start)), 0.2);
            });
    while (c)
        g_Scheduler.Run();
}

TEST(Timer, OnTime2)
{
//    g_Scheduler.GetOptions().debug = dbg_timer;
    auto start = std::chrono::system_clock::now();
    int c = 100, cc = c;
    for (float i = 0; i < c; i+=1)
        co_timer_add(std::chrono::milliseconds((int)(1000 + i * 1000/c)), [&, i]{
                --c;
                EXPECT_LT(std::abs(1 + i/cc - second_duration(start)), 0.05);
                });

    while (c)
        g_Scheduler.Run();
}

TEST(Timer, OnTime3)
{
    g_Scheduler.GetOptions().debug = dbg_none;
    int c = 10000;
    for (float i = 0; i < c; i+=1)
        co_timer_add(std::chrono::seconds(1), [&, i]{
                --c;
                });

    auto start = std::chrono::system_clock::now();
    while (c)
        g_Scheduler.Run();
    EXPECT_LT(second_duration(start), 2.0);
}

using ::testing::TestWithParam;
using ::testing::Values;

struct ThreadsTimer : public TestWithParam<int> {
    int thread_count_;
    void SetUp() { thread_count_ = GetParam(); }
};

TEST_P(ThreadsTimer, OnTime4)
{
    g_Scheduler.GetOptions().debug = dbg_none;
    std::atomic<int> c{10000};
    for (float i = 0; i < c; i+=1)
        co_timer_add(std::chrono::seconds(1), [&, i]{
                --c;
                });

    auto start = std::chrono::system_clock::now();
    boost::thread_group tg;
    tg.create_thread([&c] {
        while (c)
            g_Scheduler.Run();
            });
    tg.join_all();
    EXPECT_LT(second_duration(start), 2.0);
}

INSTANTIATE_TEST_CASE_P(OnThreadsTimer, ThreadsTimer, Values(2, 4, 8));

TEST(ThreadsTimer, Cancel)
{
    g_Scheduler.GetOptions().debug = dbg_none;

    bool executed = false;
    auto timer_id = co_timer_add(std::chrono::seconds(0), [&]{
                executed = true;
            });
    g_Scheduler.Run();
    EXPECT_TRUE(executed);

    executed = false;
    timer_id = co_timer_add(std::chrono::seconds(0), [&]{
                executed = true;
            });
    EXPECT_TRUE(co_timer_cancel(timer_id));
    g_Scheduler.Run();
    EXPECT_FALSE(executed);

    executed = false;
    timer_id = co_timer_add(std::chrono::seconds(0), [&]{
                executed = true;
            });
    g_Scheduler.Run();
    EXPECT_FALSE(co_timer_cancel(timer_id));
    EXPECT_TRUE(executed);
}

