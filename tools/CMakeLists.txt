

add_executable(snail_tool_etl etl_file.cpp common/progress_printer.cpp)
target_link_libraries(snail_tool_etl PRIVATE compile_options etl utf8cpp::utf8cpp)
set_target_properties(snail_tool_etl PROPERTIES OUTPUT_NAME "snail-tool-etl")

add_executable(snail_tool_perf_data perf_data_file.cpp common/progress_printer.cpp)
target_link_libraries(snail_tool_perf_data PRIVATE compile_options perf_data)
set_target_properties(snail_tool_perf_data PROPERTIES OUTPUT_NAME "snail-tool-perfdata")

add_executable(snail_tool_analysis analysis.cpp common/progress_printer.cpp)
target_link_libraries(snail_tool_analysis PRIVATE compile_options analysis)
set_target_properties(snail_tool_analysis PROPERTIES OUTPUT_NAME "snail-tool-analysis")

install(
  TARGETS
    snail_tool_etl
    snail_tool_perf_data
    snail_tool_analysis
  COMPONENT
    tools
  RUNTIME DESTINATION bin
)
if(SNAIL_INSTALL_PDB)
  install(
    FILES
      "$<TARGET_PDB_FILE:snail_tool_etl>"
      "$<TARGET_PDB_FILE:snail_tool_perf_data>"
      "$<TARGET_PDB_FILE:snail_tool_analysis>"
    COMPONENT
      tools
    DESTINATION "bin"
    OPTIONAL
  )
endif()
