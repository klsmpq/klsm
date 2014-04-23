add_subdirectory(bench)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(globallock STATIC
    globallock.cpp
)

add_library(lsm STATIC
    lsm.cpp
)
