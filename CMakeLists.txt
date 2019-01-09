cmake_minimum_required(VERSION 3.5)
project(Gross)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

if(GROSS_ENABLE_UNIT_TESTS)
  enable_testing()
  add_subdirectory(third_party/googletest)
  include_directories(third_party/googletest/googletest/include)
  include(GoogleTest)
  # Though there is add_compile_definitions in newer
  # cmake, we use the legacy add_definitions for compatibility
  add_definitions(-DGROSS_ENABLE_UNIT_TESTS)
endif()

add_subdirectory(src)

if(GROSS_ENABLE_UNIT_TESTS)
  add_custom_target(check-units
    COMMAND ${CMAKE_CTEST_COMMAND})
  add_dependencies(check-units unittests)
endif()
