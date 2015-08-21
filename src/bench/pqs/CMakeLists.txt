include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/sequence_heap
    ${CMAKE_SOURCE_DIR}/lib/skip_list
)

add_library(sequence_heap STATIC
    ${CMAKE_SOURCE_DIR}/lib/sequence_heap/knheap.C
    sequence_heap.cpp
)
set_target_properties(sequence_heap PROPERTIES COMPILE_FLAGS
    "-w"
)

add_library(skip_queue STATIC
    skip_queue.cpp
)
