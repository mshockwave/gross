cmake_minimum_required(VERSION 3.5)
project(Gross)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND
   GROSS_ENABLE_INSTRUMENTS)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Add clang sanitizers...")
    set(_GROSS_CLANG_SANITIZERS
        -fsanitize=address -fsanitize=undefined)
    add_compile_options(${_GROSS_CLANG_SANITIZERS})
    link_libraries(${_GROSS_CLANG_SANITIZERS})
  endif()
endif()

if(GROSS_ENABLE_UNIT_TESTS)
  enable_testing()
  add_subdirectory(third_party/googletest)
  include_directories(third_party/googletest/googletest/include)
  include(GoogleTest)
  # Though there is add_compile_definitions in newer
  # cmake, we use the legacy add_definitions for compatibility
  add_definitions(-DGROSS_ENABLE_UNIT_TESTS)
endif()

# Boost
include_directories(third_party/boost/1.68.0/include)

add_subdirectory(src)

if(GROSS_ENABLE_UNIT_TESTS)
  add_custom_target(check-units
    COMMAND ${CMAKE_CTEST_COMMAND} -R ".*UnitTest\..*")
  add_dependencies(check-units unittests)

  add_custom_target(check-integrations
    COMMAND ${CMAKE_CTEST_COMMAND} -R ".*IntegrateTest\..*")
  add_dependencies(check-integrations integration_tests)
endif()

# Execute all the tests
add_custom_target(check-gross)
if(GROSS_ENABLE_UNIT_TESTS)
  add_dependencies(check-gross
                   check-units check-integrations)
endif()

