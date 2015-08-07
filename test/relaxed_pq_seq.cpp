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

#include "k_lsm/k_lsm.h"
#include "shared_lsm/shared_lsm.h"

#define DEFAULT_SEED (0)
#define PQ_SIZE ((1 << 15) - 1)

using namespace kpq;

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

        m_elements.reserve(n);
        for (int i = 0; i < n; i++) {
            const uint32_t v = rand_int(gen);

            m_elements.push_back(v);
            m_pq->insert(v, v);
        }

        std::sort(m_elements.begin(), m_elements.end());
    }

    /** Returns the relaxed upper bound for the key returned by i'th delete_min
     *  operation. */
    virtual uint32_t relaxed_upper_bound(const int i)
    {
        return m_elements[std::min(i + RELAXATION, (int)m_elements.size() - 1)];
    }

    virtual void
    TearDown()
    {
        delete m_pq;
    }

protected:
    T *m_pq;
    std::vector<uint32_t> m_elements;
};

typedef ::testing::Types< k_lsm<uint32_t, uint32_t, RELAXATION>
                        , shared_lsm<uint32_t, uint32_t, RELAXATION>
                        > TestTypes;
TYPED_TEST_CASE(PQTest, TestTypes);

TYPED_TEST(PQTest, SanityCheck)
{
    uint32_t v;
    EXPECT_TRUE(this->m_pq->delete_min(v));
    EXPECT_LE(v, this->relaxed_upper_bound(0));
}

TYPED_TEST(PQTest, NewMinElem)
{
    constexpr static int ITERATIONS = 64;
    uint32_t v;
    for (int i = 0; i < ITERATIONS; i++) {
        ASSERT_TRUE(this->m_pq->delete_min(v));
    }

    const uint32_t w = v - 1;
    this->m_pq->insert(w, w);
    EXPECT_TRUE(this->m_pq->delete_min(v));
    EXPECT_LE(v, this->relaxed_upper_bound(ITERATIONS));
}

TYPED_TEST(PQTest, ExtractAll)
{
    uint32_t v;
    for (int i = 0; i < PQ_SIZE; i++) {
        ASSERT_TRUE(this->m_pq->delete_min(v));
        ASSERT_LE(v, this->relaxed_upper_bound(i));
    }

    ASSERT_FALSE(this->m_pq->delete_min(v));
}

TYPED_TEST(PQTest, ExtractAllDiffSizes)
{
    std::vector<int> sizes { 1, 5, 7, 15, 16, 47, 64, 48, 113, 128, 1234, 12 };
    for (int size : sizes) {
        this->generate_elements(size);

        uint32_t v;
        for (int i = 0; i < size; i++) {
            ASSERT_TRUE(this->m_pq->delete_min(v));
            ASSERT_LE(v, this->relaxed_upper_bound(i));
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
