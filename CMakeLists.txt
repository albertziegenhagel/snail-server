cmake_minimum_required(VERSION 3.25)

project(snail_support VERSION 0.1 LANGUAGES CXX C)

# =======
# Modules

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

include(CMakeDependentOption)

include(CodeCoverage)

# =======
# Options

option(BUILD_TESTING "Whether to build testing" ON)
cmake_dependent_option(SNAIL_ENABLE_CODE_COVERAGE "Enable code coverage for tests" OFF BUILD_TESTING OFF)

option(SNAIL_WITH_LLVM "Required to resolve symbols from PDB and DWARF files." ON)

# =======
# Dependencies

if(SNAIL_WITH_LLVM)
  find_package(LLVM CONFIG REQUIRED
    COMPONENTS
      LLVMDebugInfoPDB
      LLVMDebugInfoDWARF
  )
endif()

find_package(nlohmann_json CONFIG REQUIRED)

find_package(libzippp CONFIG REQUIRED)

find_package(utf8cpp CONFIG REQUIRED)

# =======
# Compile options

set(CMAKE_CXX_EXTENSIONS OFF)
include(cmake/compile_options.cmake)

if(SNAIL_ENABLE_CODE_COVERAGE)
  enable_code_coverage()
endif()

# =======
# Targets

add_subdirectory(snail)

# =======
# Tests

if(BUILD_TESTING)
  enable_testing()

  add_subdirectory(third-party/gtest)

  add_subdirectory(tests/unit)

  # Just for testing/development
  add_subdirectory(apps)
endif()

string(REGEX REPLACE "([][+.*()^])" "\\\\\\1" _SOURCE_DIR_REGEX "${PROJECT_SOURCE_DIR}")
code_coverage_report(
  EXCLUDE "${_SOURCE_DIR_REGEX}/tests/"
)
