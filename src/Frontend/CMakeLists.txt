
set(_SOURCE_FILES
    Lexer.cpp
    )

add_library(GrossFrontend OBJECT
            ${_SOURCE_FILES})

if(GROSS_BUILD_UNIT_TESTS)
  set(_TEST_SOURCE_FILES
      LexerTest.cpp
      )

  add_executable(GrossFrontendTest
                 ${_TEST_SOURCE_FILES})
  target_link_libraries(GrossFrontendTest
                        $<TARGET_OBJECTS:GrossFrontend>
                        gtest_main)
  gtest_add_tests(TARGET GrossFrontendTest)
endif()
