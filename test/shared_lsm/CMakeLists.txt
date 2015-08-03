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

add_executable(versioned-array-ptr-test versioned_array_ptr.cpp)
target_link_libraries(versioned-array-ptr-test
    gtest
    thread_local_ptr
)
add_test(NAME versioned-array-ptr-test COMMAND versioned-array-ptr-test)
