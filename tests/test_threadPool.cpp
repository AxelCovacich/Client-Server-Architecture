#include "threadPool.hpp"
#include "unity.h"
#include <atomic>

void testThreadpoolExecutesTask() {
    std::atomic<int> flag{0};
    ThreadPool pool(2);

    pool.enqueueTask([&flag]() { flag = 42; });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pool.stop();

    TEST_ASSERT_EQUAL_INT(42, flag.load());
}