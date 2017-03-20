add_subdirectory(pqs)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions("-flto")
endif()

add_executable(heapsort heapsort.cpp util.cpp)
target_link_libraries(heapsort
    ${HWLOC_LIBRARIES}
    sequence_heap
    sequential_lsm
    skip_queue
)

add_executable(random random.cpp itree.cpp util.cpp)
target_link_libraries(random
    ${CMAKE_THREAD_LIBS_INIT}
    ${HWLOC_LIBRARIES}
    capq
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

add_executable(file_shortest_paths file_shortest_paths.cpp util.cpp)
target_link_libraries(file_shortest_paths
    ${CMAKE_THREAD_LIBS_INIT}
    ${HWLOC_LIBRARIES}
    linden
    sequence_heap
    sequential_lsm
    skip_queue
    thread_local_ptr
    spraylist
    capq
    #/usr/local/lib/libpapi.a
)

add_executable(generate_random_graph generate_random_graph.cpp)
