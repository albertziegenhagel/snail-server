
add_library(analysis)

target_sources(analysis
  PRIVATE
    detail/etl_file_process_context.cpp
    detail/perf_data_file_process_context.cpp

    detail/pdb_resolver.cpp
    detail/dwarf_resolver.cpp

    detail/download.cpp

    path_map.cpp

    data_provider.cpp
    etl_data_provider.cpp
    diagsession_data_provider.cpp
    perf_data_data_provider.cpp
    
    analysis.cpp
)

target_link_libraries(analysis
  PRIVATE
    compile_options

    etl 
    perf_data
    
    libzippp::libzippp

    utf8cpp

    CURL::libcurl
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

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Workaround for https://github.com/llvm/llvm-project/issues/59784 :
    # Compile the files that include LLVM headers with C++20 flags only.
    set_source_files_properties(detail/pdb_resolver.cpp PROPERTIES COMPILE_OPTIONS ${CMAKE_CXX20_STANDARD_COMPILE_OPTION})
    set_source_files_properties(detail/dwarf_resolver.cpp PROPERTIES COMPILE_OPTIONS ${CMAKE_CXX20_STANDARD_COMPILE_OPTION})
  endif()
endif()

target_include_directories(analysis SYSTEM PRIVATE
  ${LLVM_INCLUDE_DIRS}
)

target_include_directories(analysis PUBLIC ${CMAKE_SOURCE_DIR})

code_coverage_add_target(analysis)
