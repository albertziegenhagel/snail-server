
add_library(common)

target_sources(common
  PRIVATE
    trim.cpp
    filename.cpp
    string_compare.cpp
    guid.cpp
)

target_link_libraries(common 
  PRIVATE
    compile_options
)

target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR})

code_coverage_add_target(common)
