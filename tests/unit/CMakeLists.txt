
include(GoogleTest)

add_library(test_main STATIC
  main.cpp
  folders.cpp
)
target_link_libraries(test_main compile_options GTest::gtest)
target_include_directories(test_main
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

function(add_unit_test_executable TEST_NAME)
  cmake_parse_arguments(args "" "" "SOURCES;DEPENDENCIES" ${ARGN} )
  add_executable(test_${TEST_NAME})

  target_sources(test_${TEST_NAME}
    PRIVATE
      ${args_SOURCES}
  )

  target_link_libraries(test_${TEST_NAME}
    compile_options
    test_main
    GTest::gtest
    ${args_DEPENDENCIES}
  )

  target_include_directories(test_${TEST_NAME}
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}"
  )

  cmake_path(NATIVE_PATH CMAKE_SOURCE_DIR native_source_dir)
  gtest_discover_tests(test_${TEST_NAME}
    EXTRA_ARGS "--snail-root-dir=${native_source_dir}"
  )
endfunction()

add_unit_test_executable(common
  SOURCES
    common/utility.cpp
  DEPENDENCIES
    common
)

add_unit_test_executable(etl
  SOURCES
    etl/parser.cpp
    etl/event_observer.cpp
  DEPENDENCIES
    etl
)

add_unit_test_executable(perf_data
  SOURCES
    perf_data/parser.cpp
    perf_data/event_observer.cpp
  DEPENDENCIES
    perf_data
)

add_unit_test_executable(analysis
  SOURCES
    analysis/module_map.cpp
  DEPENDENCIES
    analysis
)

add_unit_test_executable(jsonrpc
  SOURCES
    jsonrpc/request.cpp
  DEPENDENCIES
    jsonrpc
)
if(WIN32)
    target_sources(test_jsonrpc PRIVATE jsonrpc/pipe.cpp)
endif()
if(UNIX)
    target_sources(test_jsonrpc PRIVATE jsonrpc/unix_domain_socket.cpp)
endif()