include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(heapsort heapsort.cpp)
target_link_libraries(heapsort
    globallock
    lsm
)
