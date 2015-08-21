add_subdirectory(shared_lsm)
add_subdirectory(util)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIR}
)

add_executable(pq-par-test pq_par.cpp)
target_link_libraries(pq-par-test
    gtest
    thread_local_ptr
)
add_test(NAME pq-par-test COMMAND pq-par-test)

add_executable(pq-seq-test pq_seq.cpp)
target_link_libraries(pq-seq-test
    gtest
    skip_queue
    sequence_heap
    sequential_lsm
    thread_local_ptr
)
add_test(NAME pq-seq-test COMMAND pq-seq-test)

add_executable(relaxed-pq-seq-test relaxed_pq_seq.cpp)
target_link_libraries(relaxed-pq-seq-test
    gtest
    thread_local_ptr
)
add_test(NAME relaxed-pq-seq-test COMMAND relaxed-pq-seq-test)
