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
#include <random>
#include <vector>
#include <thread>

#include "shared_lsm_clean/versioned_array_ptr.h"

using namespace kpq;

#define ARRAY_ALIGNMENT (2048)
#define MASK (ARRAY_ALIGNMENT - 1)
#define NTHREADS (48)

TEST(VersionedArrayPtrTest, SanityCheck)
{
    versioned_array_ptr<uint32_t, uint32_t, ARRAY_ALIGNMENT> ptr;
}

TEST(VersionedArrayPtrTest, Sequential)
{
    versioned_array_ptr<uint32_t, uint32_t, ARRAY_ALIGNMENT> ptr;

    auto old_ptr = ptr.load_packed();
    ASSERT_NE(nullptr, old_ptr);
    ASSERT_EQ(0, ((intptr_t)old_ptr) & MASK);

    aligned_block_array<uint32_t, uint32_t, ARRAY_ALIGNMENT> new_array;
    ASSERT_TRUE(ptr.compare_exchange_strong(old_ptr, new_array));

    ASSERT_EQ(new_array.ptr(), ptr.load());
    ASSERT_EQ(new_array.ptr()->version(), ((intptr_t)ptr.load_packed()) & MASK);
}

static void
compare_exchange_local(std::atomic<bool> *can_continue,
                       versioned_array_ptr<uint32_t, uint32_t, ARRAY_ALIGNMENT> *ptr)
{
    constexpr int N = 1024;

    aligned_block_array<uint32_t, uint32_t, ARRAY_ALIGNMENT> new_array;
    block_array<uint32_t, uint32_t> *old_array;

    while (!can_continue->load()) { }

    for (int i = 0; i < N; i++) {
        do {
            old_array = ptr->load_packed();
            new_array.ptr()->copy_from(ptr->unpack(old_array));
            new_array.ptr()->increment_version();
        } while (!ptr->compare_exchange_strong(old_array, new_array));
    }
}

TEST(VersionedArrayPtrTest, ParallelReuse)
{
    versioned_array_ptr<uint32_t, uint32_t, ARRAY_ALIGNMENT> ptr;

    std::vector<std::thread> threads;
    std::atomic<bool> can_continue(false);

    for (int i = 0; i < NTHREADS; i++) {
        threads.push_back(std::thread(compare_exchange_local, &can_continue, &ptr));
    }

    can_continue.store(true, std::memory_order_relaxed);

    for (auto &thread : threads) {
        thread.join();
    }
}

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
