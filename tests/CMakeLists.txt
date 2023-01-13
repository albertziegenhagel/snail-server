
add_executable(test_etl_parser etl/parser.cpp)
target_link_libraries(test_etl_parser compile_options)
target_link_libraries(test_etl_parser etl)
target_link_libraries(test_etl_parser third_party::gtest third_party::gtest_main)
add_test(NAME test_etl_parser COMMAND test_etl_parser)

add_executable(test_etl_event_observer etl/event_observer.cpp)
target_link_libraries(test_etl_event_observer compile_options)
target_link_libraries(test_etl_event_observer etl)
target_link_libraries(test_etl_event_observer third_party::gtest third_party::gtest_main)
add_test(NAME test_etl_event_observer COMMAND test_etl_event_observer)


add_executable(test_etlreader etlreader.cpp)
target_link_libraries(test_etlreader compile_options)
target_link_libraries(test_etlreader etl)
target_link_libraries(test_etlreader third_party::gtest third_party::gtest_main)
add_test(NAME test_etlreader COMMAND test_etlreader)

add_executable(test_etl_file etl_file.cpp)
target_link_libraries(test_etl_file compile_options)
target_link_libraries(test_etl_file etl)
target_link_libraries(test_etl_file third_party::gtest third_party::gtest_main)
add_test(NAME test_etl_file COMMAND test_etl_file)

add_executable(test_main main.cpp)
target_link_libraries(test_main third_party::gtest third_party::gtest_main)
add_test(NAME test_main COMMAND test_main)