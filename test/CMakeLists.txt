include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIR}
)

add_executable(block-array-test block_array.cpp)
target_link_libraries(block-array-test
    gtest
    thread_local_ptr
)
add_test(NAME block-array-test COMMAND block-array-test)

add_executable(lockfree-vector-test lockfree_vector.cpp)
target_link_libraries(lockfree-vector-test
    gtest
    thread_local_ptr
)
add_test(NAME lockfree-vector-test COMMAND lockfree-vector-test)

add_executable(mm-test mm.cpp)
target_link_libraries(mm-test
    gtest
)
add_test(NAME mm-test COMMAND mm-test)

add_executable(pq-par-test pq_par.cpp)
target_link_libraries(pq-par-test
    gtest
    thread_local_ptr
)
add_test(NAME pq-par-test COMMAND pq-par-test)

add_executable(pq-seq-test pq_seq.cpp)
target_link_libraries(pq-seq-test
    gtest
    sequential_lsm
    thread_local_ptr
)
add_test(NAME pq-seq-test COMMAND pq-seq-test)

add_executable(thread-local-ptr-test thread_local_ptr.cpp)
target_link_libraries(thread-local-ptr-test
    gtest
    thread_local_ptr
)
add_test(NAME thread-local-ptr-test COMMAND thread-local-ptr-test)
