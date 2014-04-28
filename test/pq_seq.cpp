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

#include "globallock.h"
#include "lsm.h"

#define DEFAULT_SEED (0)
#define PQ_SIZE ((1 << 15) - 1)

using namespace kpq;

template <class T>
class PQTest : public ::testing::Test
{
protected:
    virtual void
    SetUp()
    {
        generate_elements(PQ_SIZE);
    }

    virtual void generate_elements(const int n)
    {
        std::mt19937 gen(DEFAULT_SEED);
        std::uniform_int_distribution<> rand_int;

        m_elements.clear();
        m_pq.clear();

        m_min = std::numeric_limits<uint32_t>::max();

        m_elements.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            const uint32_t v = rand_int(gen);

            m_elements.push_back(v);
            m_pq.insert(v);
            m_min = std::min(m_min, v);
        }
    }

    virtual void
    TearDown()
    {
    }

protected:
    T m_pq;
    std::vector<uint32_t> m_elements;

    uint32_t m_min;
};

typedef ::testing::Types<GlobalLock, LSM<uint32_t>> TestTypes;
TYPED_TEST_CASE(PQTest, TestTypes);

TYPED_TEST(PQTest, SanityCheck)
{
    uint32_t v;
    EXPECT_TRUE(this->m_pq.delete_min(v));
    EXPECT_EQ(this->m_min, v);
}

TYPED_TEST(PQTest, NewMinElem)
{
    uint32_t v;
    for (int i = 0; i < 64; i++) {
        ASSERT_TRUE(this->m_pq.delete_min(v));
    }

    const uint32_t w = v - 1;
    this->m_pq.insert(w);
    EXPECT_TRUE(this->m_pq.delete_min(v));
    EXPECT_EQ(w, v);
}

TYPED_TEST(PQTest, ExtractAll)
{
    uint32_t v, w;
    ASSERT_TRUE(this->m_pq.delete_min(v));
    for (int i = 1; i < PQ_SIZE; i++) {
        w = v;
        ASSERT_TRUE(this->m_pq.delete_min(v));
        ASSERT_LE(w, v);
    }

    ASSERT_FALSE(this->m_pq.delete_min(v));
}

TYPED_TEST(PQTest, ExtractAllDiffSizes)
{
    std::vector<int> sizes { 1, 5, 7, 15, 16, 47, 64, 48, 113, 128, 1234, 12 };
    for (int size : sizes) {
        this->generate_elements(size);

        uint32_t v, w;
        ASSERT_TRUE(this->m_pq.delete_min(v));
        for (int i = 1; i < size; i++) {
            w = v;
            ASSERT_TRUE(this->m_pq.delete_min(v));
            ASSERT_LE(w, v);
        }

        ASSERT_FALSE(this->m_pq.delete_min(v));
    }
}

/* Doesn't play nice with typed test case.
static void
delete_n(GlobalLock *pq,
         const uint32_t n)
{
    uint32_t v;
    for (uint32_t i = 0; i < n; i++) {
        ASSERT_TRUE(pq->delete_min(v));
    }
}

TYPED_TEST(PQTest, ThreadSanityCheck)
{
    const uint32_t n = std::thread::hardware_concurrency();

    std::vector<std::thread> ts;
    for (uint32_t i = 0; i < n; i++) {
        ts.push_back(std::thread(delete_n, &this->m_pq, PQ_SIZE / n));
    }

    for (uint32_t i = 0; i < n; i++) {
        ts[i].join();
    }
}
*/

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
