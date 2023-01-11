
add_executable(test_etlreader etlreader.cpp)
target_link_libraries(test_etlreader compile_options)
target_link_libraries(test_etlreader etlreader)
target_link_libraries(test_etlreader third_party::gtest third_party::gtest_main)
add_test(NAME test_etlreader COMMAND test_etlreader)

add_executable(test_etlfile etlfile.cpp)
target_link_libraries(test_etlfile compile_options)
target_link_libraries(test_etlfile etlreader)
target_link_libraries(test_etlfile third_party::gtest third_party::gtest_main)
add_test(NAME test_etlfile COMMAND test_etlfile)

add_executable(test_main main.cpp)
target_link_libraries(test_main third_party::gtest third_party::gtest_main)
add_test(NAME test_main COMMAND test_main)