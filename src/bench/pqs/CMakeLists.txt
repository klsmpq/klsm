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
