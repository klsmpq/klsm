include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIR}
)

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

add_executable(thread-local-ptr-test thread_local_ptr.cpp)
target_link_libraries(thread-local-ptr-test
    gtest
    thread_local_ptr
)
add_test(NAME thread-local-ptr-test COMMAND thread-local-ptr-test)
