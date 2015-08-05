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

#include "shared_lsm/aligned_block_array.h"
#include "shared_lsm/block_array.h"

using namespace kpq;

#define ARRAY_ALIGNMENT (2048)
#define MASK (ARRAY_ALIGNMENT - 1)
#define RELAXATION (32)

typedef block_array<uint32_t, uint32_t, RELAXATION> default_block_array;

TEST(BlockArrayTest, SanityCheck)
{
    default_block_array bs;
    aligned_block_array<uint32_t, uint32_t, ARRAY_ALIGNMENT> cs;
}

TEST(BlockArrayTest, Insert)
{
    default_block_array bs;

    auto b = new block<uint32_t, uint32_t>(1);

    auto i = new item<uint32_t, uint32_t>();
    i->initialize(42, 42);

    b->set_used();
    b->insert(i, i->version());

    block_pool<uint32_t, uint32_t> pool;
    bs.insert(b, &pool);

    delete i;
    delete b;
}

TEST(BlockArrayTest, Copy)
{
    default_block_array bs;

    auto b = new block<uint32_t, uint32_t>(1);

    auto i = new item<uint32_t, uint32_t>();
    i->initialize(42, 42);

    b->set_used();
    b->insert(i, i->version());

    block_pool<uint32_t, uint32_t> pool;
    bs.insert(b, &pool);

    default_block_array cs;
    cs.copy_from(&bs);

    delete i;
    delete b;
}

TEST(BlockArrayTest, DeleteMinEmpty)
{
    default_block_array bs;

    uint32_t x;
    ASSERT_FALSE(bs.delete_min(x));
}

TEST(BlockArrayTest, DeleteMin)
{
    default_block_array bs;

    auto b = new block<uint32_t, uint32_t>(1);

    auto i = new item<uint32_t, uint32_t>();
    i->initialize(42, 42);

    b->set_used();
    b->insert(i, i->version());

    block_pool<uint32_t, uint32_t> pool;
    bs.insert(b, &pool);

    uint32_t x;
    ASSERT_TRUE(bs.delete_min(x));
    ASSERT_EQ(42, x);

    delete i;
    delete b;
}

TEST(BlockArrayTest, Algn)
{
    aligned_block_array<uint32_t, uint32_t, ARRAY_ALIGNMENT> aligned_bs;
    auto bs = aligned_bs.ptr();

    ASSERT_NE(nullptr, bs);
    ASSERT_EQ(0, ((intptr_t)bs) & MASK);
}

TEST(BlockArrayTest, AlignedDeleteMin)
{
    aligned_block_array<uint32_t, uint32_t, ARRAY_ALIGNMENT> aligned_bs;
    auto bs = aligned_bs.ptr();

    auto b = new block<uint32_t, uint32_t>(1);

    auto i = new item<uint32_t, uint32_t>();
    i->initialize(42, 42);

    b->set_used();
    b->insert(i, i->version());

    block_pool<uint32_t, uint32_t> pool;
    bs->insert(b, &pool);

    uint32_t x;
    ASSERT_TRUE(bs->delete_min(x));
    ASSERT_EQ(42, x);

    delete i;
    delete b;
}

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
