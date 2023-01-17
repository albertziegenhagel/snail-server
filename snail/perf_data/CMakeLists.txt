
add_library(perf_data)

target_sources(perf_data PRIVATE
  perf_data_file.cpp
  dispatching_event_observer.cpp
  
  detail/metadata.cpp
  detail/attributes_database.cpp

  parser/records/kernel.cpp
)

target_link_libraries(perf_data compile_options)

target_include_directories(perf_data PUBLIC ${CMAKE_SOURCE_DIR})
