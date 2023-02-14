
include(GoogleTest)

add_executable(test_etl
    etl/parser.cpp
    etl/event_observer.cpp
)
target_link_libraries(test_etl compile_options etl)
target_link_libraries(test_etl third_party::gtest third_party::gtest_main)
gtest_discover_tests(test_etl)


add_executable(test_perf_data
    perf_data/parser.cpp
    perf_data/event_observer.cpp
)
target_link_libraries(test_perf_data compile_options perf_data)
target_link_libraries(test_perf_data third_party::gtest third_party::gtest_main)
gtest_discover_tests(test_perf_data)

add_executable(test_analysis
    analysis/module_map.cpp
)
target_link_libraries(test_analysis compile_options analysis)
target_link_libraries(test_analysis third_party::gtest third_party::gtest_main)
gtest_discover_tests(test_analysis)

add_executable(test_jsonrpc
    jsonrpc/request.cpp
)
if(WIN32)
    target_sources(test_jsonrpc PRIVATE jsonrpc/pipe.cpp)
endif()
if(UNIX)
    target_sources(test_jsonrpc PRIVATE jsonrpc/unix_domain_socket.cpp)
endif()
target_link_libraries(test_jsonrpc compile_options jsonrpc)
target_link_libraries(test_jsonrpc third_party::gtest third_party::gtest_main)
gtest_discover_tests(test_jsonrpc)
