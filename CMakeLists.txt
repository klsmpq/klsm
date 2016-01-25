project(kpqueue)
cmake_minimum_required(VERSION 2.8.0)

find_package(Doxygen)
find_package(Threads REQUIRED)

include(FindPkgConfig)
pkg_search_module(HWLOC REQUIRED hwloc)
pkg_search_module(VALGRIND valgrind)

add_definitions("-Wall -Wextra -pedantic -std=c++11")
add_definitions("-lstdc++")

if(DOXYGEN_FOUND)
    add_subdirectory(doc)
endif()

if(VALGRIND_FOUND)
    add_definitions("-DHAVE_VALGRIND")
endif()

add_subdirectory(src)

if(EXISTS /usr/src/gtest)
    enable_testing()
    add_subdirectory(/usr/src/gtest gtest)
    add_subdirectory(test)
endif()
