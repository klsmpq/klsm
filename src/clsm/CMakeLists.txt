include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_library(clsm STATIC
    block.cpp
    block_storage.cpp
    clsm.cpp
)
target_link_libraries(clsm
    thread_local_ptr
)
