
function(setup_mocha_test_dir)
  cmake_parse_arguments(args "" "OUT_DEPENDS_VAR;DIRECTORY" "" ${ARGN})

  set(_dir "${CMAKE_CURRENT_SOURCE_DIR}/${args_DIRECTORY}")

  set(_output "${_dir}/node_modules/.package-lock.json")

  add_custom_command(
    COMMAND "${Npm_EXECUTABLE}" install
    WORKING_DIRECTORY "${_dir}"
    OUTPUT
      "${_output}"
    DEPENDS
      "${_dir}/package.json"
      "${_dir}/package-lock.json"
  )
 
  list(APPEND ${args_OUT_DEPENDS_VAR} "${_output}")
  set(${args_OUT_DEPENDS_VAR} "${${args_OUT_DEPENDS_VAR}}" PARENT_SCOPE)
endfunction()

function(add_mocha_test)
  cmake_parse_arguments(args "" "NAME;DIRECTORY" "SPECS" ${ARGN})

  set(_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/${args_DIRECTORY}")
  set(_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${args_DIRECTORY}")
  set(_temp_dir "${_binary_dir}/temp")

  file(MAKE_DIRECTORY "${_binary_dir}")
  file(MAKE_DIRECTORY "${_temp_dir}")

  set(_specs)
  foreach(spec IN LISTS args_SPECS)
    list(APPEND _specs "${_source_dir}/${spec}")
  endforeach()

  add_test(
    NAME "${args_NAME}"
    COMMAND
      "${Node_EXECUTABLE}"
        "${_source_dir}/node_modules/ts-mocha/bin/ts-mocha"
          --require "${_source_dir}/tests/server.ts"
          -p "${_source_dir}/tsconfig.json"
          ${_specs}
    WORKING_DIRECTORY "${_binary_dir}"
  )

  set(_environment)

  list(APPEND _environment "SNAIL_SERVER_EXE=$<TARGET_FILE:snailserver>")
  cmake_path(NATIVE_PATH CMAKE_SOURCE_DIR native_source_dir)
  list(APPEND _environment "SNAIL_ROOT_DIR=${native_source_dir}")
  cmake_path(NATIVE_PATH _temp_dir native_temp_dir)
  list(APPEND _environment "SNAIL_TEMP_DIR=${native_temp_dir}")

  file(MAKE_DIRECTORY "${_binary_dir}/code_coverage")

  if(SNAIL_ENABLE_CODE_COVERAGE)
    string(REPLACE ":" "_" file_name "${args_NAME}")
    list(APPEND _environment "LLVM_PROFILE_FILE=code_coverage/${file_name}.profraw")
  endif()

  set_tests_properties("${args_NAME}"
    PROPERTIES
      ENVIRONMENT "${_environment}"
  )
endfunction()
