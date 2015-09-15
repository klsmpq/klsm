project(kpqueue)
cmake_minimum_required(VERSION 2.8.0)

find_package(Doxygen)
find_package(Threads REQUIRED)

include(FindPkgConfig)
pkg_search_module(GSL REQUIRED gsl)
pkg_search_module(HWLOC REQUIRED hwloc)
pkg_search_module(VALGRIND valgrind)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions("-Wall -Wextra -pedantic -std=c++11")
endif()

# Hacks needed for link-time optimizations (-flto) on mars / debian.
set(CMAKE_AR "gcc-ar")
set(CMAKE_RANLIB "gcc-ranlib")

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
