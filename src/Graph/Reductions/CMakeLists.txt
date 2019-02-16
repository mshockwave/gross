
set(_SOURCE_FILES
    ValuePromotion.cpp
    CSE.cpp
    Peephole.cpp
    )

add_library(GrossGraphReductions OBJECT
    ${_SOURCE_FILES})

if(GROSS_ENABLE_UNIT_TESTS)
  set(_TEST_SOURCE_FILES
      ValuePromotionTest.cpp
      CSETest.cpp
      PeepholeTest.cpp
      )

  add_executable(GrossGraphReductionsTest
    ${_TEST_SOURCE_FILES})
  target_link_libraries(GrossGraphReductionsTest
    $<TARGET_OBJECTS:GrossGraphReductions>
    $<TARGET_OBJECTS:GrossGraph>
    gtest_main)
  gtest_add_tests(TARGET GrossGraphReductionsTest)
  add_dependencies(unittests GrossGraphReductionsTest)
endif()
