include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIR}
)

add_executable(mm-test mm.cpp)
target_link_libraries(mm-test
    gtest
)
add_test(NAME mm-test COMMAND mm-test)

add_executable(pq-seq-test pq_seq.cpp)
target_link_libraries(pq-seq-test
    globallock
    gtest
    lsm
)
add_test(NAME pq-seq-test COMMAND pq-seq-test)
