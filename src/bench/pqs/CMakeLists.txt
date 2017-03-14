include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(linden STATIC
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/linden_common.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/linden.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/gc/gc.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/gc/ptst.c
    linden.cpp
)
execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE CACHE_LINE_SIZE)
set_target_properties(linden PROPERTIES COMPILE_FLAGS
    "-w -std=gnu11 -DINTEL -DCACHE_LINE_SIZE=${CACHE_LINE_SIZE}"
)
target_include_directories(linden PUBLIC
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/include
)

add_library(sequence_heap STATIC
    ${CMAKE_SOURCE_DIR}/lib/sequence_heap/knheap.C
    sequence_heap.cpp
)
set_target_properties(sequence_heap PROPERTIES COMPILE_FLAGS
    "-w"
)
target_include_directories(sequence_heap PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/sequence_heap
)

add_library(skip_queue STATIC
    skip_queue.cpp
)
target_include_directories(skip_queue PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/skip_list
)

add_library(spraylist STATIC
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/fraser.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/intset.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/linden_common.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/linden.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/measurements.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/pqueue.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/skiplist.c
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/ssalloc.c
    spraylist.cpp
)
set_target_properties(spraylist PROPERTIES COMPILE_FLAGS
    "-w -std=gnu11 -DINTEL -DLOCKFREE -DCACHE_LINE_SIZE=${CACHE_LINE_SIZE}"
)
target_include_directories(spraylist PUBLIC
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/atomic_ops
    ${CMAKE_SOURCE_DIR}/lib/spraylist_linden/include
)

add_library(capq STATIC
    ${CMAKE_SOURCE_DIR}/lib/capq/capq.c
    ${CMAKE_SOURCE_DIR}/lib/capq/fat_skiplist.c
    ${CMAKE_SOURCE_DIR}/lib/capq/qdlocksrc/locks/qd_lock.c
    ${CMAKE_SOURCE_DIR}/lib/capq/qdlocksrc/locks/tatas_lock.c
    ${CMAKE_SOURCE_DIR}/lib/capq/gc/gc.c
    ${CMAKE_SOURCE_DIR}/lib/capq/gc/ptst.c
)
set_target_properties(capq PROPERTIES COMPILE_FLAGS
    "-w -std=gnu11 -DINTEL -DCACHE_LINE_SIZE=${CACHE_LINE_SIZE}"
)
target_include_directories(capq PUBLIC
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/capq
    ${CMAKE_SOURCE_DIR}/lib/capq/qdlocksrc
)
