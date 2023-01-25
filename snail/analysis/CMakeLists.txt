
add_library(analysis)

target_sources(analysis PRIVATE
  etl_stack_provider.cpp

  call_tree.cpp

  detail/module_map.cpp
  detail/etl_file_process_context.cpp
  detail/pdb_resolver.cpp

  
  # etl_builder.cpp
  # reader.cpp
)

target_link_libraries(analysis
  compile_options
  common
  data
  etl 
  perf_data

  LLVMDebugInfoPDB
)

target_include_directories(analysis SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)

target_include_directories(analysis PUBLIC ${CMAKE_SOURCE_DIR})
