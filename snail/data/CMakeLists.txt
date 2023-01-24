
add_library(data)

target_sources(data PRIVATE
  call_tree.cpp
)

target_link_libraries(data compile_options)

target_include_directories(data PUBLIC ${CMAKE_SOURCE_DIR})
