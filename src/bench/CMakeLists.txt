include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(heapsort heapsort.cpp util.cpp)
target_link_libraries(heapsort
    globallock
    lsm
    sequence_heap
    skip_queue
)

add_executable(random random.cpp util.cpp)
target_link_libraries(random
    ${CMAKE_THREAD_LIBS_INIT}
    clsm
    globallock
    linden
    lsm
    sequence_heap
    skip_queue
)
