
cmake_minimum_required(VERSION 3.25)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        f8d7d77c06936315286eb55f8de22cd23c188571 # v1.14.0
  SYSTEM
)

FetchContent_MakeAvailable(googletest)

set_target_properties(gtest gmock
  PROPERTIES
    COMPILE_WARNING_AS_ERROR OFF
)
