
add_library(etl)

target_sources(etl PRIVATE
  etl_file.cpp
  dispatching_event_observer.cpp
  
  # etlreader.cpp
)

target_link_libraries(etl compile_options)

# target_link_libraries(etl DiaSDK::DiaSDK)

target_include_directories(etl PUBLIC ${CMAKE_SOURCE_DIR})
