add_subdirectory(bench)
add_subdirectory(sequence_heap)
add_subdirectory(skip_list)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(globallock STATIC
    globallock.cpp
)

add_library(lsm STATIC
    lsm.cpp
)

add_library(thread_local_ptr STATIC
    lockfree_vector.cpp
    thread_local_ptr.cpp
)

