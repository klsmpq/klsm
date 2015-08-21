add_subdirectory(pqs)

include_directories(
    ${CMAKE_SOURCE_DIR}/lib/skip_list
    ${CMAKE_SOURCE_DIR}/src
    /opt/hwloc/include/
)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions("-flto")
endif()

add_executable(heapsort heapsort.cpp util.cpp)
target_link_libraries(heapsort
    /opt/hwloc/lib/libhwloc.so
    sequence_heap
    sequential_lsm
    skip_queue
)

add_executable(random random.cpp itree.cpp util.cpp)
target_link_libraries(random
    ${CMAKE_THREAD_LIBS_INIT}
    /opt/hwloc/lib/libhwloc.so
    sequence_heap
    sequential_lsm
    skip_queue
    thread_local_ptr
)

add_executable(shortest_paths shortest_paths.cpp util.cpp)
target_link_libraries(shortest_paths
    ${CMAKE_THREAD_LIBS_INIT}
    /opt/hwloc/lib/libhwloc.so
    sequence_heap
    sequential_lsm
    skip_queue
    thread_local_ptr
)
