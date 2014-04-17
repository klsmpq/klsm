#include <gtest/gtest.h>
#include <vector>
#include <thread>

#include "globallock.h"

#define PQ_SIZE (1 << 15)

using namespace kpq;

class PQTest : public ::testing::Test
{
protected:
    virtual void
    SetUp()
    {
        m_elements.reserve(PQ_SIZE);
        for (uint32_t i = 0; i < PQ_SIZE; i++) {
            m_elements.push_back(i);
            m_pq.insert(i);
        }
    }

    virtual void
    TearDown()
    {
    }

protected:
    GlobalLock m_pq;
    std::vector<uint32_t> m_elements;
};

TEST_F(PQTest, SanityCheck)
{
    uint32_t v;
    EXPECT_TRUE(m_pq.delete_min(v));
    EXPECT_EQ(0, v);
}

TEST_F(PQTest, NewMinElem)
{
    uint32_t v;
    for (int i = 0; i < 64; i++) {
        EXPECT_TRUE(m_pq.delete_min(v));
    }

    const uint32_t w = v - 1;
    m_pq.insert(w);
    EXPECT_TRUE(m_pq.delete_min(v));
    EXPECT_EQ(w, v);
}

TEST_F(PQTest, ExtractAll)
{
    uint32_t v;
    for (int i = 0; i < PQ_SIZE; i++) {
        EXPECT_TRUE(m_pq.delete_min(v));
    }

    EXPECT_FALSE(m_pq.delete_min(v));
}

static void
delete_n(GlobalLock *pq,
         const uint32_t n)
{
    uint32_t v;
    for (uint32_t i = 0; i < n; i++) {
        ASSERT_TRUE(pq->delete_min(v));
    }
}

TEST_F(PQTest, ThreadSanityCheck)
{
    const uint32_t n = std::thread::hardware_concurrency();

    std::vector<std::thread> ts;
    for (uint32_t i = 0; i < n; i++) {
        ts.push_back(std::thread(delete_n, &m_pq, PQ_SIZE / n));
    }

    for (uint32_t i = 0; i < n; i++) {
        ts[i].join();
    }
}

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
