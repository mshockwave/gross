
set(_SOURCE_FILES
    GraphScheduling.cpp
    InstructionSelector.cpp
    RegisterAllocator.cpp
    )

add_library(GrossCodeGen OBJECT
  ${_SOURCE_FILES})
