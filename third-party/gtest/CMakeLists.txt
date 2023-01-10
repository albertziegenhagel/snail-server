
cmake_minimum_required(VERSION 3.5.0)

option(USE_SYSTEM_GOOGLETEST "Use system googletest and do not download/build an own version of it" OFF)

if(USE_SYSTEM_GOOGLETEST)

  find_package(GTest REQUIRED)

  set_target_properties(GTest::GTest PROPERTIES
    IMPORTED_GLOBAL TRUE
  )
  set_target_properties(GTest::Main PROPERTIES
    IMPORTED_GLOBAL TRUE
  )

  add_library(third_party::gtest INTERFACE IMPORTED GLOBAL)
  add_library(third_party::gtest_main INTERFACE IMPORTED GLOBAL)

  set_target_properties(third_party::gtest PROPERTIES
    INTERFACE_LINK_LIBRARIES GTest::GTest
  )
  set_target_properties(third_party::gtest_main PROPERTIES
    INTERFACE_LINK_LIBRARIES GTest::Main
  )
else()
  project(3rd-party-googletest NONE)

  if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
  endif()

  include(ExternalProject)

  set(GTEST_FORCE_SHARED_CRT ON)
  set(GTEST_DISABLE_PTHREADS OFF)

  if(MINGW)
    set(GTEST_DISABLE_PTHREADS ON)
  endif()

  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()

  # set(GTEST_INTERFACE_COMPILE_DEFINITIONS "GTEST_LANG_CXX11")

  # set(ADDITIONAL_ARGS)
  # if(WIN32)
  #   list(APPEND ADDITIONAL_ARGS PATCH_COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/internal_utils.cmake.patched" <SOURCE_DIR>/googletest/cmake/internal_utils.cmake)
  #   list(APPEND GTEST_INTERFACE_COMPILE_DEFINITIONS "_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING")
  # endif()

  ExternalProject_Add(gtest-external
    URL https://github.com/google/googletest/archive/release-1.12.1.tar.gz
    TIMEOUT 60
    CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=Debug
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=Release
      -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO:PATH=RelWithDebInfo
      -Dgtest_force_shared_crt=${GTEST_FORCE_SHARED_CRT}
      -Dgtest_disable_pthreads=${GTEST_DISABLE_PTHREADS}
      -DBUILD_GTEST=ON
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    BUILD_BYPRODUCTS
      "gtest-external-prefix/src/gtest-external-build/googletest/${CMAKE_BUILD_TYPE}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
      "gtest-external-prefix/src/gtest-external-build/googletest/${CMAKE_BUILD_TYPE}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
    ${ADDITIONAL_ARGS}
  )

  set_target_properties(gtest-external PROPERTIES FOLDER 3rd_party)

  ExternalProject_Get_Property(gtest-external source_dir)
  set(GTEST_INCLUDE_DIRS "${source_dir}/googletest/include")

  ExternalProject_Get_Property(gtest-external binary_dir)
  set(GTEST_LIBRARY_DIR "${binary_dir}/googletest")

  file(MAKE_DIRECTORY ${GTEST_INCLUDE_DIRS})

  add_library(third_party::gtest IMPORTED STATIC GLOBAL)
  add_dependencies(third_party::gtest gtest-external)
  set_target_properties(third_party::gtest PROPERTIES
      IMPORTED_LOCATION "${GTEST_LIBRARY_DIR}/${CMAKE_BUILD_TYPE}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
      IMPORTED_LOCATION_DEBUG "${GTEST_LIBRARY_DIR}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
      IMPORTED_LOCATION_RELEASE "${GTEST_LIBRARY_DIR}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
      IMPORTED_LOCATION_RELWITHDEBINFO "${GTEST_LIBRARY_DIR}/RelWithDebInfo/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
      IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
      INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIRS}"
      INTERFACE_COMPILE_DEFINITIONS "${GTEST_INTERFACE_COMPILE_DEFINITIONS}"
  )

  add_library(third_party::gtest_main IMPORTED STATIC GLOBAL)
  add_dependencies(third_party::gtest_main gtest-external)
  set_target_properties(third_party::gtest_main PROPERTIES
      IMPORTED_LOCATION "${GTEST_LIBRARY_DIR}/${CMAKE_BUILD_TYPE}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
      IMPORTED_LOCATION_DEBUG "${GTEST_LIBRARY_DIR}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
      IMPORTED_LOCATION_RELEASE "${GTEST_LIBRARY_DIR}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
      IMPORTED_LOCATION_RELWITHDEBINFO "${GTEST_LIBRARY_DIR}/RelWithDebInfo/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
      IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
      INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIRS}"
  )
endif()
