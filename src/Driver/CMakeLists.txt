#FIXME: use better header file structure
include_directories(${CMAKE_SOURCE_DIR}/src)

set(_SOURCE_FILES
    GrossDriver.cpp
    )

add_executable(gross
  ${_SOURCE_FILES})
target_link_libraries(gross
  $<TARGET_OBJECTS:GrossFrontend>
  $<TARGET_OBJECTS:GrossGraph>
  $<TARGET_OBJECTS:GrossGraphReductions>
  $<TARGET_OBJECTS:GrossCodeGen>
  cxxopts)
