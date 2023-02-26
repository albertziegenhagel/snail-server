
add_library(common)

target_sources(common
  PRIVATE
    unicode.cpp
    trim.cpp
    filename.cpp
)

target_link_libraries(common 
  PRIVATE
    compile_options
)

target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR})
