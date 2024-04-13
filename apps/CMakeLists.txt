

add_executable(app_etl_file etl_file.cpp)
target_link_libraries(app_etl_file PRIVATE compile_options etl utf8cpp)

add_executable(app_perf_data_file perf_data_file.cpp)
target_link_libraries(app_perf_data_file PRIVATE compile_options perf_data)

add_executable(app_analysis analysis.cpp)
target_link_libraries(app_analysis PRIVATE compile_options analysis)

install(
  TARGETS
    app_etl_file
    app_perf_data_file
    app_analysis
  COMPONENT
    tools
  RUNTIME DESTINATION bin
)
