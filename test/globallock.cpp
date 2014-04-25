#include <gtest/gtest.h>
#include <vector>
#include <thread>

#include "globallock.h"
#include "lsm.h"

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
        m_elements.clear();
        m_pq.clear();

        m_elements.reserve(n);
        for (uint32_t i = 0; i < n; i++) {
            m_elements.push_back(i);
            m_pq.insert(i);
        }
    }

    virtual void
    TearDown()
    {
    }

protected:
    T m_pq;
    std::vector<uint32_t> m_elements;
};

typedef ::testing::Types<GlobalLock, LSM<uint32_t>> TestTypes;
TYPED_TEST_CASE(PQTest, TestTypes);

TYPED_TEST(PQTest, SanityCheck)
{
    uint32_t v;
    EXPECT_TRUE(this->m_pq.delete_min(v));
    EXPECT_EQ(0, v);
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
