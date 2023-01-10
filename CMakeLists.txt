cmake_minimum_required(VERSION 3.20)

project(perfreader VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 23)

include(cmake/compile_options.cmake)

find_package(DiaSDK REQUIRED)

# =======
# LIBRARIES

add_subdirectory(src)

# =======
# TESTS

enable_testing()

add_subdirectory(third-party/gtest)

add_subdirectory(tests)