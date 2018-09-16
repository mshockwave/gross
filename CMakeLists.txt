cmake_minimum_required(VERSION 3.5)
project(Gross)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

if(GROSS_BUILD_UNIT_TESTS)
  enable_testing()
  add_subdirectory(third_party/googletest)
  include_directories(third_party/googletest/googletest/include)
  include(GoogleTest)
endif()

add_subdirectory(src)
