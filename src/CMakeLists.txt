add_subdirectory(bench)
add_subdirectory(sequential_lsm)
add_subdirectory(util)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# A silly hack to make all files (including headers) show up in QtCreator.
file(GLOB_RECURSE ALL_HEADERS *.h)
add_library(qtcreator_dummy ${ALL_HEADERS} util/thread_local_ptr.cpp)
