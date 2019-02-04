
set(_SOURCE_FILES
    Graph.cpp
    Node.cpp
    NodeUtils.cpp
    GraphReducer.cpp
    )

add_library(GrossGraph OBJECT
  ${_SOURCE_FILES})

if(GROSS_ENABLE_UNIT_TESTS)
  set(_TEST_SOURCE_FILES
      NodeUtilsTest.cpp
      BGLTest.cpp
      GraphTest.cpp
      )

  add_executable(GrossGraphTest
    ${_TEST_SOURCE_FILES})
  target_link_libraries(GrossGraphTest
    $<TARGET_OBJECTS:GrossFrontend>
    $<TARGET_OBJECTS:GrossGraph>
    gtest_main)
  gtest_add_tests(TARGET GrossGraphTest)
  add_dependencies(unittests GrossGraphTest)
endif()

add_subdirectory(Reductions)
