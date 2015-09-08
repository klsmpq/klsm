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

#include "dist_lsm/dist_lsm.h"
#include "k_lsm/k_lsm.h"
#include "shared_lsm/shared_lsm.h"

#define DEFAULT_SEED (0)
#define PQ_SIZE ((1 << 15) - 1)
#define NTHREADS (160)
#define NELEMS (1024)

using namespace kpq;

#define RELAXATION (32)

template <class T>
class pq_par_test : public ::testing::Test
{
protected:
    virtual void
    SetUp()
    {
        m_pq = nullptr;
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

typedef ::testing::Types< dist_lsm<uint32_t, uint32_t, RELAXATION>
                        , k_lsm<uint32_t, uint32_t, RELAXATION>
                        , shared_lsm<uint32_t, uint32_t, RELAXATION>
                        > test_types;
TYPED_TEST_CASE(pq_par_test, test_types);

TYPED_TEST(pq_par_test, SanityCheck)
{
    this->generate_elements(42);

    uint32_t v;
    EXPECT_TRUE(this->m_pq->delete_min(v));
    ASSERT_TRUE(this->m_pq->supports_concurrency());
}

template <class T>
static void
random_insert(T *pq,
              const int seed,
              const int n)
{
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> rand_int;

    for (int i = 0; i < n; i++) {
        pq->insert(rand_int(gen));
    }
}

TYPED_TEST(pq_par_test, ConcurrentInsert)
{
    this->generate_elements(42);

    std::vector<std::thread> threads(NTHREADS);
    std::atomic<bool> can_continue(false);

    for (int i = 0; i < NTHREADS; i++) {
        threads[i] = std::thread(random_insert<gtest_TypeParam_>,
                                 this->m_pq,
                                 i,
                                 NELEMS);
    }

    can_continue.store(true, std::memory_order_relaxed);

    for (auto &thread : threads) {
        thread.join();
    }
}

template <class T>
static void
random_delete(T *pq,
              const int n)
{
    for (int i = 0; i < n; i++) {
        uint32_t v;
        pq->delete_min(v);
        /* We can't give any guarantees as to ordering of deleted values
         * because of spy(). */
    }
}

TYPED_TEST(pq_par_test, ConcurrentDelete)
{
    this->generate_elements(NELEMS * NTHREADS);

    std::vector<std::thread> threads(NTHREADS);
    std::atomic<bool> can_continue(false);

    for (int i = 0; i < NTHREADS; i++) {
        threads[i] = std::thread(random_delete<gtest_TypeParam_>,
                                 this->m_pq,
                                 NELEMS);
    }

    can_continue.store(true, std::memory_order_relaxed);

    for (auto &thread : threads) {
        thread.join();
    }
}

template <class T>
static void
random_ins_del(T *pq,
               const int seed,
               const int n)
{
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> rand_int;
    std::uniform_int_distribution<> rand_bool(0, 1);

    for (int i = 0; i < n; i++) {
        if (rand_bool(gen)) {
            uint32_t v;
            pq->delete_min(v);
        } else {
            pq->insert(rand_int(gen));
        }
    }
}

TYPED_TEST(pq_par_test, ConcurrentInsDel)
{
    this->generate_elements(NTHREADS);

    std::vector<std::thread> threads(NTHREADS);
    std::atomic<bool> can_continue(false);

    for (int i = 0; i < NTHREADS; i++) {
        threads[i] = std::thread(random_ins_del<gtest_TypeParam_>,
                                 this->m_pq,
                                 i,
                                 NELEMS);
    }

    can_continue.store(true, std::memory_order_relaxed);

    for (auto &thread : threads) {
        thread.join();
    }
}

template <class T>
static void
random_delete_strict(T *pq,
                     const int n)
{
    uint32_t prev = std::numeric_limits<uint32_t>::min();
    for (int i = 0; i < n; i++) {
        uint32_t v;
        ASSERT_TRUE(pq->delete_min(v));
        ASSERT_LE(prev, v);
        prev = v;
    }
}

template <class T>
static void
random_ins_del_same_thread(T *pq,
                           const int seed,
                           const int n)
{
    random_insert(pq, seed, n);
    random_delete_strict(pq, n);
}

TYPED_TEST(pq_par_test, ConcurrentInsDelSameThread)
{
    if (typeid(gtest_TypeParam_) == typeid(shared_lsm<uint32_t, uint32_t, RELAXATION>)
            || typeid(gtest_TypeParam_) == typeid(k_lsm<uint32_t, uint32_t, RELAXATION>)) {
        return;  // TODO: The shared lsm does not preserve local consistency.
    }
    this->generate_elements(NELEMS);

    std::vector<std::thread> threads(NTHREADS);
    std::atomic<bool> can_continue(false);

    for (int i = 0; i < NTHREADS; i++) {
        threads[i] = std::thread(random_ins_del_same_thread<gtest_TypeParam_>,
                                 this->m_pq,
                                 i,
                                 NELEMS);
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
