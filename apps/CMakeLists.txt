

add_executable(app_etl_file etl_file.cpp)
target_link_libraries(app_etl_file compile_options etl)

add_executable(app_perf_data_file perf_data_file.cpp)
target_link_libraries(app_perf_data_file compile_options perf_data)

add_executable(app_analysis analysis.cpp)
target_link_libraries(app_analysis compile_options analysis)
