#include <gtest/gtest.h>
#include <vector>

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

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
