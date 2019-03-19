
# path relative to top-level source path
include_directories(${CMAKE_SOURCE_DIR}/src)

set(_TEST_SOURCE_FILES
    ValueAssignmentTest.cpp
    MemoryTest.cpp
    FullPipelineTest.cpp
    )
set(_TEST_INPUT_FILES
    value_assignment1.txt
    value_assignment2.txt
    value_assignment3.txt
    memory1.txt
    memory2.txt
    full_pipeline1.txt
    full_pipeline2.txt
    full_pipeline3.txt
    )

if(GROSS_ENABLE_UNIT_TESTS)
  # Copy the test input files
  list(TRANSFORM _TEST_INPUT_FILES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/
       OUTPUT_VARIABLE _DEST_TEST_INPUT_FILES)

  add_custom_command(OUTPUT ${_DEST_TEST_INPUT_FILES}
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy ${_TEST_INPUT_FILES} ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${_TEST_INPUT_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  add_custom_target(integrate_test_files
    DEPENDS ${_DEST_TEST_INPUT_FILES})

  add_executable(GrossIntegrationTest
    ${_TEST_SOURCE_FILES})

  add_dependencies(GrossIntegrationTest integrate_test_files)
  target_link_libraries(GrossIntegrationTest
    $<TARGET_OBJECTS:GrossFrontend>
    $<TARGET_OBJECTS:GrossGraph>
    $<TARGET_OBJECTS:GrossGraphReductions>
    $<TARGET_OBJECTS:GrossCodeGen>
    gtest_main)
  gtest_add_tests(TARGET GrossIntegrationTest)

  add_dependencies(integration_tests GrossIntegrationTest)
endif()
