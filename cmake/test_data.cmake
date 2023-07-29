
function(add_extract_diagsession_command NAME)
  cmake_parse_arguments(args "" "OUTPUT_VAR;BUILD_TYPE;ETL_DIR;COUNTERS_DIR;COUNTERS_FILE" "BYPRODUCTS" ${ARGN})

  set(_record_dir "${CMAKE_SOURCE_DIR}/tests/apps/${NAME}/dist/windows/${args_BUILD_TYPE}/record")
  set(_extract_dir "${CMAKE_BINARY_DIR}/tests/extract/${NAME}/${args_BUILD_TYPE}")
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
