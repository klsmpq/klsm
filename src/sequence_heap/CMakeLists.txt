add_library(sequence_heap STATIC
    knheap.C
    sequence_heap.cpp
)
# Silence all warnings we can't control.
set_target_properties(sequence_heap PROPERTIES COMPILE_FLAGS
    "-w"
)
