
cmake_minimum_required(VERSION 3.25)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        b796f7d44681514f58a683a3a71ff17c94edb0c1 # v1.13.0
  SYSTEM
)

FetchContent_MakeAvailable(googletest)

set_target_properties(gtest gmock
  PROPERTIES
    COMPILE_WARNING_AS_ERROR OFF
)
