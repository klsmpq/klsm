include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
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
