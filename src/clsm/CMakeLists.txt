include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_library(clsm STATIC
    clsm.cpp
)
target_link_libraries(clsm
    thread_local_ptr
)
