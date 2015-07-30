include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(thread_local_ptr STATIC
    thread_local_ptr.cpp
)
