
function(add_test_executable TEST_NAME)
  cmake_parse_arguments(args "" "PREFIX" "SOURCES;DEPENDENCIES" ${ARGN} )
  add_executable(test_${TEST_NAME})

  target_sources(test_${TEST_NAME}
    PRIVATE
      ${args_SOURCES}
  )

  target_link_libraries(test_${TEST_NAME}
    PRIVATE
      compile_options
      test_main
      GTest::gtest
      GTest::gmock
      ${args_DEPENDENCIES}
  )

  target_include_directories(test_${TEST_NAME}
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}"
  )

  code_coverage_add_target(test_${TEST_NAME})

  cmake_path(NATIVE_PATH CMAKE_SOURCE_DIR native_source_dir)
  gtest_discover_tests(test_${TEST_NAME}
    EXTRA_ARGS "--snail-root-dir=${native_source_dir}"
    TEST_PREFIX "${args_PREFIX}"
  )
  code_coverage_setup_discovered_tests(test_${TEST_NAME})
endfunction()
