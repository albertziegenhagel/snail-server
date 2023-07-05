
add_library(common)

target_sources(common
  PRIVATE
    filename.cpp
    string_compare.cpp
    guid.cpp
    system.cpp
    trim.cpp
)

target_link_libraries(common 
  PRIVATE
    compile_options

    utf8cpp
)

target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR})

code_coverage_add_target(common)
