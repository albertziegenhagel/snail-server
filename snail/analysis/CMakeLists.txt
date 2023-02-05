
add_library(analysis)

target_sources(analysis
  PRIVATE
    detail/module_map.cpp

    detail/etl_file_process_context.cpp
    detail/perf_data_file_process_context.cpp

    detail/pdb_resolver.cpp
    detail/dwarf_resolver.cpp

    etl_stack_provider.cpp
    perf_data_stack_provider.cpp
    
    call_tree.cpp
)

target_link_libraries(analysis
  PRIVATE
    compile_options

    etl 
    perf_data

    LLVMDebugInfoPDB
    LLVMDebugInfoDWARF
  PUBLIC
    common
    data
)

target_include_directories(analysis SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)

target_include_directories(analysis PUBLIC ${CMAKE_SOURCE_DIR})
