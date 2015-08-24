add_subdirectory(sequence_heap)
add_subdirectory(skip_list)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(linden STATIC
    ${CMAKE_SOURCE_DIR}/lib/linden/common.c
    ${CMAKE_SOURCE_DIR}/lib/linden/prioq.c
    ${CMAKE_SOURCE_DIR}/lib/linden/gc/gc.c
    ${CMAKE_SOURCE_DIR}/lib/linden/gc/ptst.c
    linden.cpp
)
execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE CACHE_LINE_SIZE)
set_target_properties(linden PROPERTIES COMPILE_FLAGS
    "-w -std=c99 -DINTEL -DCACHE_LINE_SIZE=${CACHE_LINE_SIZE}"
)
target_include_directories(linden PUBLIC
    ${CMAKE_SOURCE_DIR}/lib
)

add_library(spraylist STATIC
    ${CMAKE_SOURCE_DIR}/lib/spraylist/fraser.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/intset.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/linden_common.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/linden.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/measurements.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/pqueue.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/skiplist.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist/ssalloc.c
    spraylist.cpp
)
set_target_properties(spraylist PROPERTIES COMPILE_FLAGS
    "-w -std=c99 -DINTEL -DLOCKFREE -DCACHE_LINE_SIZE=${CACHE_LINE_SIZE}"
)
target_include_directories(spraylist PUBLIC
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/spraylist/include
)
