include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIR}
)

add_executable(globallock-test globallock.cpp)
target_link_libraries(globallock-test
    globallock
    gtest
    lsm
)
add_test(NAME globallock-test COMMAND globallock-test)
