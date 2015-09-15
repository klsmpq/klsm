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

#include "util/mm.h"

struct simple_reuse {
    bool operator()(const uint32_t &) const
    {
        return false;
    }
};

struct reuse_above_42 {
    bool operator()(const uint32_t &v) const
    {
        return (v > 42);
    }
};

TEST(MMTest, SanityCheck)
{
    kpq::item_allocator<uint32_t, simple_reuse> alloc;
}

TEST(MMTest, AllocOne)
{
    kpq::item_allocator<uint32_t, simple_reuse> alloc;
    ASSERT_NE(alloc.acquire(), nullptr);
}

TEST(MMTest, AllocMany)
{
    kpq::item_allocator<uint32_t, simple_reuse> alloc;

    for (int i = 0; i < 66; i++) {
    }
}

/**
 * Verifies that a reusable item is actually reused, and non-reusable items
 * aren't. The block size is set to ITERATIONS, which should ensure that
 * the reusable item (in this case marked as usable by having a value
 * greater than 42) is reused.
 */
TEST(MMTest, ReuseCheck)
{
    static constexpr size_t ITERATIONS = 42;
    kpq::item_allocator<uint32_t, reuse_above_42, ITERATIONS> alloc;

    std::vector<uint32_t *> xs;

    for (size_t i = 0; i < ITERATIONS; i++) {
        uint32_t *x = alloc.acquire();
        *x = i;

        /* No reuse below 42. */
        for (size_t j = 0; j < i; j++) {
            ASSERT_NE(x, xs[j]);
        }

        xs.push_back(x);
    }

    uint32_t *y = alloc.acquire();
    *y = 66;

    bool reused = false;
    for (size_t i = 0; i < ITERATIONS + 1; i++) {
        uint32_t *x = alloc.acquire();
        *x = 0;

        if (x == y) {
            reused = true;
            break;
        }
    }

    /* No reuse below 42. */
    for (size_t i = 0; i < ITERATIONS; i++) {
        ASSERT_EQ(i, *xs[i]);
    }

    ASSERT_TRUE(reused);
}

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
