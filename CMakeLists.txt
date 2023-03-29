cmake_minimum_required(VERSION 3.25)

project(snail_support VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(CMAKE_CXX_EXTENSIONS OFF)

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

add_subdirectory(tests/unit)

# Just for testing/development
add_subdirectory(apps)
