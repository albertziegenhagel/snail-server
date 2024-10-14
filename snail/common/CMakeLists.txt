
add_library(common)

target_sources(common
  PRIVATE
    filename.cpp
    guid.cpp
    path.cpp
    string_compare.cpp
    system.cpp
    thread.cpp
    thread_pool.cpp
    trim.cpp
    wildcard.cpp
    ms_xca_decompression.cpp
    progress.cpp
)

target_link_libraries(common 
  PRIVATE
    compile_options

    utf8cpp::utf8cpp
)

target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR})

code_coverage_add_target(common)
