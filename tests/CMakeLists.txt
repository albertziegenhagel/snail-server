
include(GoogleTest)

add_executable(test_etl
    etl/parser.cpp
    etl/event_observer.cpp
)
target_link_libraries(test_etl compile_options etl)
target_link_libraries(test_etl third_party::gtest third_party::gtest_main)
gtest_discover_tests(test_etl)

add_executable(test_etl_file
    etl_file.cpp
)
target_link_libraries(test_etl_file compile_options etl)
target_link_libraries(test_etl_file third_party::gtest third_party::gtest_main)
gtest_discover_tests(test_etl_file)

add_executable(test_perf_data
    perf_data/parser.cpp
    perf_data/event_observer.cpp
)
target_link_libraries(test_perf_data compile_options perf_data)
target_link_libraries(test_perf_data third_party::gtest third_party::gtest_main)
gtest_discover_tests(test_perf_data)

add_executable(test_perf_data_file perf_data_file.cpp)
target_link_libraries(test_perf_data_file compile_options perf_data)
target_link_libraries(test_perf_data_file third_party::gtest third_party::gtest_main)
gtest_discover_tests(test_perf_data_file)
