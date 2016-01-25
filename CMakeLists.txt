project(kpqueue)
cmake_minimum_required(VERSION 2.8.0)

find_package(Doxygen)
find_package(Threads REQUIRED)

include(FindPkgConfig)
pkg_search_module(HWLOC REQUIRED hwloc)
pkg_search_module(VALGRIND valgrind)

add_definitions("-Wall -Wextra -pedantic -std=c++11")
add_definitions("-lstdc++ -mmic")

set(CMAKE_EXE_LINKER_FLAGS "-mmic -lrt -L/home/students/gruber/src/hwloc/hwloc/.libs/ -L /opt/intel/composer_xe_2013_sp1.2.144/compiler/lib/mic/ ${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-mmic -lrt -L/home/students/gruber/src/hwloc/hwloc/.libs/ -L /opt/intel/composer_xe_2013_sp1.2.144/compiler/lib/mic/ ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_STATIC_LINKER_FLAGS "-mmic -lrt -L/home/students/gruber/src/hwloc/hwloc/.libs/ -L /opt/intel/composer_xe_2013_sp1.2.144/compiler/lib/mic/ ${CMAKE_STATIC_LINKER_FLAGS}")

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
