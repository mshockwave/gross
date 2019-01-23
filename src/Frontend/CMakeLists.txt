
set(_SOURCE_FILES
    Lexer.cpp
    ParseExpr.cpp
    ParseStmt.cpp
    ParseDecl.cpp
    Parser.cpp
    )

add_library(GrossFrontend OBJECT
  ${_SOURCE_FILES})

if(GROSS_ENABLE_UNIT_TESTS)
  set(_TEST_SOURCE_FILES
      LexerTest.cpp
      ParseExprTest.cpp
      ParseStmtTest.cpp
      ParseDeclTest.cpp
      ParserTest.cpp
      )

  add_executable(GrossFrontendTest
    ${_TEST_SOURCE_FILES})
  target_link_libraries(GrossFrontendTest
    $<TARGET_OBJECTS:GrossFrontend>
    $<TARGET_OBJECTS:GrossGraph>
    gtest_main)
  gtest_add_tests(TARGET GrossFrontendTest)
  add_dependencies(unittests GrossFrontendTest)
endif()
