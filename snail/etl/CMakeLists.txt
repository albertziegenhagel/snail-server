
add_library(etl)

target_sources(etl
  PRIVATE
    etl_file.cpp
    dispatching_event_observer.cpp
)

target_link_libraries(etl
  PRIVATE
    compile_options
  PUBLIC
    common
)

target_include_directories(etl PUBLIC ${CMAKE_SOURCE_DIR})

code_coverage_add_target(etl)
