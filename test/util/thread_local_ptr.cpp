/*
 *  Copyright 2014 Jakob Gruber
 *
 *  This file is part of kpqueue.
 *
 *  kpqueue is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  kpqueue is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with kpqueue.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <thread>

#include "util/thread_local_ptr.h"

TEST(ThreadLocalPtrTest, SanityCheck)
{
    kpq::thread_local_ptr<uint32_t> p;
    ASSERT_NE(p.get(), nullptr);
}

static void
write_local(kpq::thread_local_ptr<uint32_t> *p,
            const int i,
            std::atomic<bool> *can_continue,
            uint32_t **result)
{
    while (!can_continue->load(std::memory_order_relaxed)) {
        /* Try to start all threads more or less at once to encourage collisions. */
    }

    uint32_t *x = p->get();
    ASSERT_NE(x, nullptr);

    *x = i;
    *result = x;
}

TEST(ThreadLocalPtrTest, ManyThreads)
{
    constexpr static int NTHREADS = 1024;

    kpq::thread_local_ptr<uint32_t> p;

    std::vector<std::thread> threads(NTHREADS);
    std::atomic<bool> can_continue(false);

    uint32_t *results[NTHREADS];

    for (int i = 0; i < NTHREADS; i++) {
        threads[i] = std::thread(write_local, &p, i, &can_continue, &results[i]);
    }

    can_continue.store(true, std::memory_order_relaxed);

    for (auto &thread : threads) {
        thread.join();
    }

    for (int i = 0; i < NTHREADS; i++) {
        ASSERT_EQ(*results[i], i);
    }
}

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
