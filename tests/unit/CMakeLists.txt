
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
    PRIVATE
      compile_options
      test_main
      GTest::gtest
      GTest::gmock
      ${args_DEPENDENCIES}
  )

  target_include_directories(test_${TEST_NAME}
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}"
  )

  code_coverage_add_target(test_${TEST_NAME})

  cmake_path(NATIVE_PATH CMAKE_SOURCE_DIR native_source_dir)
  gtest_discover_tests(test_${TEST_NAME}
    EXTRA_ARGS "--snail-root-dir=${native_source_dir}"
  )
  code_coverage_setup_discovered_tests(test_${TEST_NAME})
endfunction()

add_unit_test_executable(common
  SOURCES
    common/parser.cpp
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
    perf_data/utility.cpp
  DEPENDENCIES
    perf_data
)

add_unit_test_executable(analysis
  SOURCES
    analysis/dwarf_resolver.cpp
    analysis/etl_file_process_context.cpp
    analysis/module_map.cpp
    analysis/path_map.cpp
    analysis/pdb_resolver.cpp
    analysis/perf_data_file_process_context.cpp
    analysis/process_history.cpp
    analysis/stack_cache.cpp
    analysis/stacks_analysis.cpp
  DEPENDENCIES
    analysis
)

add_unit_test_executable(jsonrpc
  SOURCES
    jsonrpc/message.cpp
    jsonrpc/protocol.cpp
    jsonrpc/request.cpp
    jsonrpc/server.cpp
  DEPENDENCIES
    jsonrpc
)
if(WIN32)
    target_sources(test_jsonrpc PRIVATE jsonrpc/pipe.cpp)
endif()
if(UNIX)
    target_sources(test_jsonrpc PRIVATE jsonrpc/unix_domain_socket.cpp)
endif()
