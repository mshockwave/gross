
# path relative to top-level source path
include_directories(${CMAKE_SOURCE_DIR}/src)

set(_TEST_SOURCE_FILES
    ValueAssignmentTest.cpp
    )

if(GROSS_ENABLE_UNIT_TESTS)
  add_executable(GrossIntegrationTest
    ${_TEST_SOURCE_FILES})
  target_link_libraries(GrossIntegrationTest
    $<TARGET_OBJECTS:GrossFrontend>
    $<TARGET_OBJECTS:GrossGraph>
    $<TARGET_OBJECTS:GrossGraphReductions>
    gtest_main)
  gtest_add_tests(TARGET GrossIntegrationTest)
  add_dependencies(integration_tests GrossIntegrationTest)
endif()
