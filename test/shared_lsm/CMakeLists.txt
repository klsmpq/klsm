include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIR}
)

add_executable(shared-lsm-block-array-test block_array.cpp)
target_link_libraries(shared-lsm-block-array-test
    gtest
    thread_local_ptr
)
add_test(NAME shared-lsm-block-array-test COMMAND shared-lsm-block-array-test)

add_executable(shared-lsm-versioned-array-ptr-test versioned_array_ptr.cpp)
target_link_libraries(shared-lsm-versioned-array-ptr-test
    gtest
    thread_local_ptr
)
add_test(NAME shared-lsm-versioned-array-ptr-test COMMAND shared-lsm-versioned-array-ptr-test)
