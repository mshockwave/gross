
set(_SOURCE_FILES
    ValuePromotion.cpp
    )

add_library(GrossGraphReductions OBJECT
    ${_SOURCE_FILES})
target_link_libraries(GrossGraphReductions
    $<TARGET_OBJECTS:GrossGraph>)

if(GROSS_ENABLE_UNIT_TESTS)
  set(_TEST_SOURCE_FILES
      ValuePromotionTest.cpp
      )

  add_executable(GrossGraphReductionsTest
    ${_TEST_SOURCE_FILES})
  target_link_libraries(GrossGraphReductionsTest
    $<TARGET_OBJECTS:GrossGraphReductions>
    gtest_main)
  gtest_add_tests(TARGET GrossGraphReductionsTest)
  add_dependencies(unittests GrossGraphReductionsTest)
endif()
