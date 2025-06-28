cmake_minimum_required(VERSION 3.25)

project(snail_support VERSION 0.1 LANGUAGES CXX C)

# =======
# Modules

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

include(CMakeDependentOption)

include(CodeCoverage)

# =======
# Options

option(BUILD_TESTING "Whether to build unit & integration tests" ON)
cmake_dependent_option(SNAIL_ENABLE_CODE_COVERAGE "Enable code coverage for tests" OFF BUILD_TESTING OFF)

option(SNAIL_BUILD_TOOLS "Whether to build some debugging tools" ON)

option(SNAIL_WITH_LLVM "Required to resolve symbols from PDB and DWARF files." ON)

option(SNAIL_ENABLE_SYSTEMTESTS "Whether to enable system testing" ON)

option(SNAIL_INSTALL_PDB "Install PDB files with the DLLs" OFF)

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

find_package(CURL CONFIG REQUIRED)

if(BUILD_TESTING)
  add_subdirectory(third-party/gtest)
endif()

if(SNAIL_ENABLE_SYSTEMTESTS)
  find_package(Node REQUIRED)

  find_package(Npm REQUIRED)
endif()

# =======
# Compile options

set(CMAKE_CXX_EXTENSIONS OFF)
include(cmake/compile_options.cmake)

if(SNAIL_ENABLE_CODE_COVERAGE)
  enable_code_coverage()
endif()

if(BUILD_TESTING OR SNAIL_ENABLE_SYSTEMTESTS)
  enable_testing()

  include(cmake/test_data.cmake)

  add_extract_diagsession_command(inner
    OUTPUT_VAR    inner_deb_etl_file
    BUILD_TYPE    deb
    ETL_DIR       "10B4DD17-F140-4CA2-BB1A-4CCC7AF0F691"
    COUNTERS_DIR  "617A03BA-77C7-44E3-8040-566B171F3ADA"
    COUNTERS_FILE "8822D5E9-64DD-5269-B4F5-5387BD6C2FCB"
  )

  add_custom_target(extract_test_data
    DEPENDS
      "${inner_deb_etl_file}"
  )
endif()

# =======
# Targets

add_subdirectory(snail)

if(SNAIL_BUILD_TOOLS)
  # Just for testing/development
  add_subdirectory(tools)
endif()

# =======
# Tests

if(BUILD_TESTING)
  include(GoogleTest)

  include(cmake/test_executable.cmake)

  add_subdirectory(tests/common)
  add_subdirectory(tests/unit)
  add_subdirectory(tests/integration)
endif()

if(SNAIL_ENABLE_SYSTEMTESTS)
  include(cmake/test_mocha.cmake)

  add_subdirectory(tests/system)
endif()

set(coverage_exclude_dirs
  "tests"
  "snail/common/third_party"
)

set(coverage_exclude_regexes)
foreach(dir IN LISTS coverage_exclude_dirs)
  cmake_path(APPEND PROJECT_SOURCE_DIR "${dir}" OUTPUT_VARIABLE _exclude_path)
  cmake_path(NATIVE_PATH _exclude_path NORMALIZE _exclude_path)
  string(REGEX REPLACE "([][+.*()^\\])" "\\\\\\1" _exclude_path_regex "${_exclude_path}")
  list(APPEND coverage_exclude_regexes "${_exclude_path_regex}")
endforeach()

list(JOIN coverage_exclude_regexes "|" coverage_joined_exclude_regex)
code_coverage_report(
  EXCLUDE "${coverage_joined_exclude_regex}"
)
