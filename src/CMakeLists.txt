add_subdirectory(bench)
add_subdirectory(sequence_heap)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(globallock STATIC
    globallock.cpp
)

add_library(lsm STATIC
    lsm.cpp
)

add_library(mm STATIC
    mm.cpp
)

