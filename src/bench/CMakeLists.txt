add_subdirectory(pqs)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions("-flto")
endif()

add_executable(random random.cpp itree.cpp util.cpp)
target_link_libraries(random
    ${CMAKE_THREAD_LIBS_INIT}
    ${HWLOC_LIBRARIES}
    sequence_heap
    sequential_lsm
    skip_queue
    thread_local_ptr
)
