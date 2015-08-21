project(kpqueue)
cmake_minimum_required(VERSION 2.8.0)

find_package(Doxygen)
find_package(Threads REQUIRED)

include(FindPkgConfig)
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/gsl/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/hwloc/lib/pkgconfig")
pkg_search_module(HWLOC REQUIRED hwloc)
pkg_search_module(VALGRIND valgrind)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_definitions("-Wall -Wextra -pedantic -std=c++11 -m64")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m64")
endif()

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
