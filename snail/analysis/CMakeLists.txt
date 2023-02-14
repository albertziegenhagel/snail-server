
add_library(analysis)

target_sources(analysis
  PRIVATE
    detail/module_map.cpp

    detail/etl_file_process_context.cpp
    detail/perf_data_file_process_context.cpp

    detail/pdb_resolver.cpp
    detail/dwarf_resolver.cpp

    data_provider.cpp
    etl_data_provider.cpp
    perf_data_data_provider.cpp
    
    analysis.cpp
)

target_link_libraries(analysis
  PRIVATE
    compile_options

    etl 
    perf_data
  PUBLIC
    common
)

if(SNAIL_WITH_LLVM)
  target_link_libraries(analysis
    PRIVATE
      LLVMDebugInfoPDB
      LLVMDebugInfoDWARF
  )
  target_compile_definitions(analysis
    PUBLIC
      SNAIL_HAS_LLVM
  )
endif()

target_include_directories(analysis SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)

target_include_directories(analysis PUBLIC ${CMAKE_SOURCE_DIR})
