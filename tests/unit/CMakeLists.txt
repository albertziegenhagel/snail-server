
add_test_executable(common
  PREFIX "Unit::"
  SOURCES
    common/chunked_reader.cpp
    common/parser.cpp
    common/utility.cpp
  DEPENDENCIES
    common
)

add_test_executable(etl
  PREFIX "Unit::"
  SOURCES
    etl/parser.cpp
    etl/event_observer.cpp
  DEPENDENCIES
    etl
)

add_test_executable(perf_data
  PREFIX "Unit::"
  SOURCES
    perf_data/attributes_database.cpp
    perf_data/parser.cpp
    perf_data/event_observer.cpp
    perf_data/utility.cpp
  DEPENDENCIES
    perf_data
)

add_test_executable(analysis
  PREFIX "Unit::"
  SOURCES
    analysis/dwarf_resolver.cpp
    analysis/etl_file_process_context.cpp
    analysis/module_map.cpp
    analysis/options.cpp
    analysis/path_map.cpp
    analysis/pdb_resolver.cpp
    analysis/perf_data_file_process_context.cpp
    analysis/process_history.cpp
    analysis/stack_cache.cpp
    analysis/stacks_analysis.cpp
  DEPENDENCIES
    analysis
)

add_test_executable(jsonrpc
  PREFIX "Unit::"
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
