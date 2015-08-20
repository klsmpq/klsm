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

#include "shared_lsm/versioned_array_ptr.h"

using namespace kpq;

#define ARRAY_ALIGNMENT (2048)
#define MASK (ARRAY_ALIGNMENT - 1)
#define NTHREADS (48)
#define RELAXATION (32)

typedef block_array<uint32_t, uint32_t, RELAXATION> ba;
typedef versioned_array_ptr<uint32_t, uint32_t, RELAXATION, ARRAY_ALIGNMENT> vap;
typedef aligned_block_array<uint32_t, uint32_t, RELAXATION, ARRAY_ALIGNMENT> aba;

TEST(VersionedArrayPtrTest, SanityCheck)
{
    vap ptr;
}

TEST(VersionedArrayPtrTest, Sequential)
{
    vap ptr;

    auto old_ptr = ptr.load_packed();
    auto old_version = ptr.load()->version();
    ASSERT_NE(nullptr, old_ptr);
    ASSERT_EQ(0, ptr.load()->version());
    ASSERT_EQ(old_version & MASK, ((intptr_t)old_ptr) & MASK);
    ASSERT_TRUE(vap::matches(old_ptr, old_version));

    aba new_array;
    new_array.ptr()->increment_version();
    auto new_version = new_array.ptr()->version();
    ASSERT_EQ(1, new_version);

    ASSERT_TRUE(ptr.compare_exchange_strong(old_ptr, new_array));
    auto new_ptr = ptr.load_packed();

    ASSERT_EQ(new_array.ptr(), ptr.load());
    ASSERT_EQ(new_array.ptr()->version(), ((intptr_t)new_ptr) & MASK);
    ASSERT_FALSE(vap::matches(new_ptr, old_version));
    ASSERT_TRUE(vap::matches(new_ptr, new_version));
}

static void
compare_exchange_local(std::atomic<bool> *can_continue,
                       std::atomic<int> *num_done,
                       vap *ptr)
{
    constexpr int N = 1024;

    aba new_array;
    ba *old_array;

    while (!can_continue->load()) { }

    for (int i = 0; i < N; i++) {
        do {
            old_array = ptr->load_packed();
            new_array.ptr()->copy_from(ptr->unpack(old_array));
            new_array.ptr()->increment_version();
        } while (!ptr->compare_exchange_strong(old_array, new_array));
    }

    /* Ensure memory the thread-local array isn't being freed prematurely. */

    num_done->fetch_add(1);
    while (num_done->load() != NTHREADS) { }
}

TEST(VersionedArrayPtrTest, ParallelReuse)
{
    vap ptr;

    std::vector<std::thread> threads(NTHREADS);
    std::atomic<bool> can_continue(false);
    std::atomic<int> num_done(0);

    for (int i = 0; i < NTHREADS; i++) {
        threads[i] = std::thread(compare_exchange_local,
                                 &can_continue,
                                 &num_done,
                                 &ptr);
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
