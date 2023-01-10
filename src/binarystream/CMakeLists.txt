
add_library(binstream)

target_sources(binstream PRIVATE
  binarystream.cpp
  memory_stream.cpp
)

target_link_libraries(binstream compile_options)

target_include_directories(binstream PUBLIC ..)
