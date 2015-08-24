add_subdirectory(pqs)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(heapsort heapsort.cpp util.cpp)
target_link_libraries(heapsort
    ${HWLOC_LIBRARIES}
    sequence_heap
    sequential_lsm
    skip_queue
)

add_executable(random random.cpp util.cpp)
target_link_libraries(random
    ${CMAKE_THREAD_LIBS_INIT}
    ${HWLOC_LIBRARIES}
    linden
    sequence_heap
    sequential_lsm
    skip_queue
    spraylist
    thread_local_ptr
)

add_executable(shortest_paths shortest_paths.cpp util.cpp)
target_link_libraries(shortest_paths
    ${CMAKE_THREAD_LIBS_INIT}
    ${HWLOC_LIBRARIES}
    linden
    sequence_heap
    sequential_lsm
    skip_queue
    thread_local_ptr
)
