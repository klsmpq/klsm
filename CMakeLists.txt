project(kpqueue)
cmake_minimum_required(VERSION 2.8.0)

find_package(Doxygen)
find_package(Threads)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions("-Wall -Wextra -pedantic -std=c++11")
endif()

if(DOXYGEN_FOUND)
    add_subdirectory(doc)
endif()

add_subdirectory(src)

if(EXISTS /usr/src/gtest)
    enable_testing()
    add_subdirectory(/usr/src/gtest gtest)
    add_subdirectory(test)
endif()
