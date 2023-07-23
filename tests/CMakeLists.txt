
include(GoogleTest)

add_library(test_main STATIC
  common/main.cpp
  common/folders.cpp
)
target_link_libraries(test_main compile_options GTest::gtest)
target_include_directories(test_main
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
)

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

function(add_extract_diagsession_command NAME)
  cmake_parse_arguments(args "" "OUTPUT_VAR;BUILD_TYPE;ETL_DIR;COUNTERS_DIR;COUNTERS_FILE;" "BYPRODUCTS" ${ARGN})

  set(_record_dir "${CMAKE_CURRENT_SOURCE_DIR}/apps/${NAME}/dist/windows/${args_BUILD_TYPE}/record")
  set(_extract_dir "${CMAKE_CURRENT_BINARY_DIR}/extract/${NAME}/${args_BUILD_TYPE}")
  set(_output_file "${_record_dir}/${NAME}.etl")

  add_custom_command(
    OUTPUT "${_output_file}"
    BYPRODUCTS
      "${_extract_dir}/metadata.xml"
      "${_extract_dir}/[Content_Types].xml"
      "${_extract_dir}/${args_ETL_DIR}/sc.user_aux.etl"
      "${_extract_dir}/${args_ETL_DIR}"
      "${_extract_dir}/${args_COUNTERS_DIR}/${args_COUNTERS_FILE}.counters"
      "${_extract_dir}/${args_COUNTERS_DIR}"
    WORKING_DIRECTORY "${_extract_dir}"
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${_record_dir}/${NAME}.diagsession"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${_extract_dir}/${args_ETL_DIR}/sc.user_aux.etl"
      "${_output_file}"
    COMMENT "Extract ${NAME}.diagsession (${args_BUILD_TYPE})"
  )
  set(${args_OUTPUT_VAR} "${_output_file}" PARENT_SCOPE)
endfunction()

add_extract_diagsession_command(inner
  OUTPUT_VAR    inner_deb_etl_file
  BUILD_TYPE    deb
  ETL_DIR       "10B4DD17-F140-4CA2-BB1A-4CCC7AF0F691"
  COUNTERS_DIR  "617A03BA-77C7-44E3-8040-566B171F3ADA"
  COUNTERS_FILE "8822D5E9-64DD-5269-B4F5-5387BD6C2FCB"
)

add_custom_target(extract_test_data
  DEPENDS
    "${inner_deb_etl_file}"
)

add_subdirectory(unit)
add_subdirectory(integration)
