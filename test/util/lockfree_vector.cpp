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

#include "util/lockfree_vector.h"

TEST(LockfreeVectorTest, SanityCheck)
{
    kpq::lockfree_vector<uint32_t> v;
}

TEST(LockfreeVectorTest, AllocFour)
{
    kpq::lockfree_vector<uint32_t> v;

    uint32_t *x0 = v.get(0);
    uint32_t *x1 = v.get(1);
    uint32_t *x2 = v.get(2);
    uint32_t *x3 = v.get(3);

    ASSERT_NE(x0, nullptr);
    ASSERT_NE(x1, nullptr);
    ASSERT_NE(x2, nullptr);
    ASSERT_NE(x3, nullptr);

    *x0 = 0;
    *x1 = 1;
    *x2 = 2;
    *x3 = 3;

    ASSERT_EQ(*x0, 0);
    ASSERT_EQ(*x1, 1);
    ASSERT_EQ(*x2, 2);
    ASSERT_EQ(*x3, 3);
}

TEST(LockfreeVectorTest, AllocFourRev)
{
    kpq::lockfree_vector<uint32_t> v;

    uint32_t *x3 = v.get(3);
    uint32_t *x2 = v.get(2);
    uint32_t *x1 = v.get(1);
    uint32_t *x0 = v.get(0);

    ASSERT_NE(x0, nullptr);
    ASSERT_NE(x1, nullptr);
    ASSERT_NE(x2, nullptr);
    ASSERT_NE(x3, nullptr);

    *x0 = 0;
    *x1 = 1;
    *x2 = 2;
    *x3 = 3;

    ASSERT_EQ(*x0, 0);
    ASSERT_EQ(*x1, 1);
    ASSERT_EQ(*x2, 2);
    ASSERT_EQ(*x3, 3);
}

TEST(LockfreeVectorTest, AllocHigh)
{
    kpq::lockfree_vector<uint32_t> v;

    uint32_t *x0 = v.get(0);
    uint32_t *x1 = v.get(1021);
    uint32_t *x2 = v.get(1020);

    ASSERT_NE(x0, nullptr);
    ASSERT_NE(x1, nullptr);
    ASSERT_NE(x2, nullptr);

    *x0 = 0;
    *x1 = 1;
    *x2 = 2;

    ASSERT_EQ(*x0, 0);
    ASSERT_EQ(*x1, 1);
    ASSERT_EQ(*x2, 2);
}

static void
alloc_bucket(kpq::lockfree_vector<uint32_t> *v,
             const int i,
             std::atomic<bool> *can_continue)
{
    while (!can_continue->load(std::memory_order_relaxed)) {
        /* Try to start all threads more or less at once to encourage collisions. */
    }

    uint32_t *x = v->get(i);
    *x = i;
}

TEST(LockfreeVectorTest, AllocConcurrent)
{
    constexpr static int NTHREADS = 1024;

    kpq::lockfree_vector<uint32_t> v;
    std::vector<std::thread> threads(NTHREADS);
    std::atomic<bool> can_continue(false);

    for (int i = 0; i < NTHREADS; i++) {
        threads[i] = std::thread(alloc_bucket, &v, i, &can_continue);
    }

    can_continue.store(true, std::memory_order_relaxed);

    for (auto &thread : threads) {
        thread.join();
    }

    for (int i = 0; i < NTHREADS; i++) {
        ASSERT_EQ(*v.get(i), i);
    }
}

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
