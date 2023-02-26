cmake_minimum_required(VERSION 3.20)

project(snail_support VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 23)

include(cmake/compile_options.cmake)

option(SNAIL_WITH_LLVM "Required to resolve symbols from PDB and DWARF files." ON)

if(SNAIL_WITH_LLVM)
  find_package(LLVM CONFIG REQUIRED
    COMPONENTS
      LLVMDebugInfoPDB
      LLVMDebugInfoDWARF
  )
endif()

find_package(nlohmann_json CONFIG REQUIRED)

find_package(libzippp CONFIG REQUIRED)

# =======
# LIBRARIES

add_subdirectory(snail)

# =======
# TESTS

enable_testing()

add_subdirectory(third-party/gtest)

add_subdirectory(tests)

# Just for testing/development
add_subdirectory(apps)
