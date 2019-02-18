
set(_SOURCE_FILES
    GraphScheduling.cpp
    InstructionSelector.cpp
    RegisterAllocator.cpp
    )

add_library(GrossCodeGen OBJECT
  ${_SOURCE_FILES})

if(GROSS_ENABLE_UNIT_TESTS)
  set(_TEST_SOURCE_FILES
      GraphSchedulingTest.cpp
      )

  add_executable(GrossCodeGenTest
    ${_TEST_SOURCE_FILES})
  target_link_libraries(GrossCodeGenTest
    $<TARGET_OBJECTS:GrossGraph>
    $<TARGET_OBJECTS:GrossCodeGen>
    gtest_main)
  gtest_add_tests(TARGET GrossCodeGenTest)
  add_dependencies(unittests GrossCodeGenTest)
endif()
