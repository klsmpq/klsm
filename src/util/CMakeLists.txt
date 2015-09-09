include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions("-flto")
endif()

add_library(thread_local_ptr STATIC
    thread_local_ptr.cpp
)
