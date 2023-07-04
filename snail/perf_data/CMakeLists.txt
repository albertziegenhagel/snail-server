
add_library(perf_data)

target_sources(perf_data
  PRIVATE
    build_id.cpp
    metadata.cpp
    perf_data_file.cpp
    dispatching_event_observer.cpp

    detail/attributes_database.cpp

    parser/records/kernel.cpp
)

target_link_libraries(perf_data
  PRIVATE
    compile_options
  PUBLIC
    common
)

target_include_directories(perf_data PUBLIC ${CMAKE_SOURCE_DIR})

code_coverage_add_target(perf_data)
