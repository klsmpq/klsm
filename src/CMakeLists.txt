add_subdirectory(bench)
add_subdirectory(clsm)
add_subdirectory(sequence_heap)
add_subdirectory(skip_list)

set(CFLAGS_NO_WARNINGS "-Wno-unused-parameter -Wno-type-limits -Wno-overflow -Wno-format -Wno-pedantic")
set(CFLAGS_NO_WARNINGS "${CFLAGS_NO_WARNINGS} -Wno-sign-compare -Wno-unused-but-set-variable -Wno-long-long")
set(CFLAGS_NO_WARNINGS "${CFLAGS_NO_WARNINGS} -Wno-unused-function")
set(CFLAGS_NO_WARNINGS "${CFLAGS_NO_WARNINGS} -Wno-implicit-function-declaration -Wno-unused-variable")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(globallock STATIC
    globallock.cpp
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
    "-std=c99 ${CFLAGS_NO_WARNINGS} -DINTEL -DCACHE_LINE_SIZE=${CACHE_LINE_SIZE}"
)
target_include_directories(linden PUBLIC
    ${CMAKE_SOURCE_DIR}/lib
)

add_library(lsm STATIC
    lsm.cpp
)

add_library(thread_local_ptr STATIC
    thread_local_ptr.cpp
)

