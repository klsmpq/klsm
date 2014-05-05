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
#include <random>

#include "clsm/clsm.h"

#define DEFAULT_SEED (0)
#define PQ_SIZE ((1 << 15) - 1)

using namespace kpq;

template <class T>
class pq_par_test : public ::testing::Test
{
protected:
    virtual void
    SetUp()
    {
        m_pq = nullptr;
        generate_elements(PQ_SIZE);
    }

    virtual void
    generate_elements(const int n)
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
            m_pq->insert(v);
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

typedef ::testing::Types<clsm<uint32_t>> test_types;
TYPED_TEST_CASE(pq_par_test, test_types);

TYPED_TEST(pq_par_test, SanityCheck)
{
    uint32_t v;
    EXPECT_TRUE(this->m_pq->delete_min(v));
    EXPECT_EQ(this->m_min, v);
}

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
