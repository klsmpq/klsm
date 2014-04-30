include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_library(clsm STATIC
    block.cpp
    block_storage.cpp
    clsm.cpp
    clsm_local.cpp
)
target_link_libraries(clsm
    lsm
    thread_local_ptr
)
