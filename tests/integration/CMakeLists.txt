
add_test_executable(integration_etl
  PREFIX "Integration::"
  SOURCES
    etl.cpp
  DEPENDENCIES
    etl
)
add_dependencies(test_integration_etl extract_test_data)

add_test_executable(integration_perf_data_file
  PREFIX "Integration::"
  SOURCES
    perf_data.cpp
  DEPENDENCIES
    perf_data
)

add_test_executable(integration_analysis
  PREFIX "Integration::"
  SOURCES
    analysis.cpp
  DEPENDENCIES
    analysis
)
