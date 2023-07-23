
add_test_executable(integration_analysis
  PREFIX "Integration::"
  SOURCES
    analysis.cpp
  DEPENDENCIES
    analysis
)
