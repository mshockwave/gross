
set(_SOURCE_FILES
    GrossDriver.cpp
    )

add_executable(gross
  ${_SOURCE_FILES})
target_link_libraries(gross
  $<TARGET_OBJECTS:GrossFrontend>
  $<TARGET_OBJECTS:GrossGraph>
  $<TARGET_OBJECTS:GrossGraphReductions>
  cxxopts)
