cmake_minimum_required(VERSION 3.20)

project(snail_support VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 23)

include(cmake/compile_options.cmake)

find_package(LLVM REQUIRED CONFIG
  COMPONENTS
    LLVMDebugInfoPDB
    LLVMDebugInfoDWARF
)

# =======
# LIBRARIES

add_subdirectory(snail)

# =======
# TESTS

enable_testing()

add_subdirectory(third-party/gtest)

add_subdirectory(tests)