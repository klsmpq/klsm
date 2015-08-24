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

#include "bench/pqs/globallock.h"
#include "bench/pqs/sequence_heap.h"
#include "bench/pqs/skip_queue.h"
#include "dist_lsm/dist_lsm.h"
#include "sequential_lsm/lsm.h"
#include "shared_lsm/shared_lsm.h"

#define DEFAULT_SEED (0)
#define PQ_SIZE ((1 << 15) - 1)

using namespace kpq;
using namespace kpqbench;

#define RELAXATION (32)

template <class T>
class PQTest : public ::testing::Test
{
protected:
    virtual void
    SetUp()
    {
        m_pq = nullptr;
        generate_elements(PQ_SIZE);
    }

    virtual void generate_elements(const int n)
    {
        std::mt19937 gen(DEFAULT_SEED);
        std::uniform_int_distribution<> rand_int;

        m_elements.clear();

        delete m_pq;
        m_pq = new T();

        m_min = std::numeric_limits<uint32_t>::max();

        m_elements.reserve(n);
        for (int i = 0; i < n; i++) {
            const uint32_t v = rand_int(gen);

            m_elements.push_back(v);
            m_pq->insert(v, v);
            m_min = std::min(m_min, v);
        }
    }

    virtual void
    TearDown()
    {
        delete m_pq;
    }

protected:
    T *m_pq;
    std::vector<uint32_t> m_elements;

    uint32_t m_min;
};

/* The Linden queue is not tested since it does not distinguish between
 * successful and unsuccessful delete_mins.
 */

typedef ::testing::Types< GlobalLock<uint32_t, uint32_t>
                        , LSM<uint32_t>
                        , dist_lsm<uint32_t, uint32_t, RELAXATION>
                        , sequence_heap<uint32_t>
                        , skip_queue<uint32_t>
                        > TestTypes;
TYPED_TEST_CASE(PQTest, TestTypes);

TYPED_TEST(PQTest, SanityCheck)
{
    uint32_t v;
    EXPECT_TRUE(this->m_pq->delete_min(v));
    EXPECT_EQ(this->m_min, v);
}

TYPED_TEST(PQTest, NewMinElem)
{
    uint32_t v;
    for (int i = 0; i < 64; i++) {
        ASSERT_TRUE(this->m_pq->delete_min(v));
    }

    const uint32_t w = v - 1;
    this->m_pq->insert(w, w);
    EXPECT_TRUE(this->m_pq->delete_min(v));
    EXPECT_EQ(w, v);
}

TYPED_TEST(PQTest, ExtractAll)
{
    uint32_t v, w;
    ASSERT_TRUE(this->m_pq->delete_min(v));
    for (int i = 1; i < PQ_SIZE; i++) {
        w = v;
        ASSERT_TRUE(this->m_pq->delete_min(v));
        ASSERT_LE(w, v);
    }

    ASSERT_FALSE(this->m_pq->delete_min(v));
}

TYPED_TEST(PQTest, ExtractAllDiffSizes)
{
    std::vector<int> sizes { 1, 5, 7, 15, 16, 47, 64, 48, 113, 128, 1234, 12 };
    for (int size : sizes) {
        this->generate_elements(size);

        uint32_t v, w;
        ASSERT_TRUE(this->m_pq->delete_min(v));
        for (int i = 1; i < size; i++) {
            w = v;
            ASSERT_TRUE(this->m_pq->delete_min(v));
            ASSERT_LE(w, v);
        }

        ASSERT_FALSE(this->m_pq->delete_min(v));
    }
}

TYPED_TEST(PQTest, InsDel)
{
    this->generate_elements(0);

    std::mt19937 gen(8);
    std::uniform_int_distribution<> rand_int;
    std::uniform_int_distribution<> rand_bool(0, 1);

    for (int i = 0; i < 1024; i++) {
        if (rand_bool(gen)) {
            uint32_t v;
            this->m_pq->delete_min(v);

#ifdef DEBUG
            printf("delete_min(%d):\n", v);
            this->m_pq->print();
#endif
        } else {
            const uint32_t v = rand_int(gen);
            this->m_pq->insert(v, v);

#ifdef DEBUG
            printf("insert(%d):\n", v);
            this->m_pq->print();
#endif
        }
    }
}

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
