#include <gtest/gtest.h>

#include "globallock.h"

using namespace kpq;

int
main(int argc,
     char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
