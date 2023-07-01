
function(enable_code_coverage)
  get_property(_coverage_enabled GLOBAL PROPERTY "_CodeCoverage_enabled")
  if(_coverage_enabled)
    return()
  endif()

  set(_compile_options)
  set(_link_options)
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    get_filename_component(_compiler_path "${CMAKE_CXX_COMPILER}" PATH)
    string(REGEX MATCH "^[0-9]+.[0-9]+" _llvm_version "${CMAKE_CXX_COMPILER_VERSION}")

    find_program(
      LLVM_PROFDATA_PATH
      NAMES "llvm-profdata" "llvm-profdata-${_llvm_version}"
      HINTS ${_compiler_path}
    )
    if(NOT LLVM_PROFDATA_PATH)
      message(SEND_ERROR "Missing: LLVM_PROFDATA_PATH")
    endif()

    find_program(
      LLVM_COV_PATH
      NAMES "llvm-cov" "llvm-cov-${_llvm_version}"
      HINTS ${_compiler_path}
    )
    if(NOT LLVM_COV_PATH)
      message(SEND_ERROR "Missing: LLVM_COV_PATH")
    endif()

    # set(_pass_through_prefix)
    # if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    #   set(_pass_through_prefix "/clang:")
    # endif()
    # list(APPEND _compile_options
    #   "${_pass_through_prefix}-fprofile-arcs"
    #   "${_pass_through_prefix}-ftest-coverage"
    # )
    # list(APPEND _link_options
    #   "-fprofile-arcs"
    #   "-ftest-coverage"
    # )

    list(APPEND _compile_options
      "-fprofile-instr-generate"
      "-fcoverage-mapping"
      # -mllvm -runtime-counter-relocation
    )
    list(APPEND _link_options
      "-fprofile-instr-generate"
      "-fcoverage-mapping"
      # -mllvm -runtime-counter-relocation
    )

    # Workaround and issue with picking up incorrect profiler libraries. See:
    #   https://youtrack.jetbrains.com/issue/CPP-29577
    if("${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "MSVC")
      execute_process(
          COMMAND "${CMAKE_CXX_COMPILER}" "/clang:--print-search-dirs"
          OUTPUT_VARIABLE _search_dirs
          COMMAND_ERROR_IS_FATAL ANY
      )
      string(STRIP "${_search_dirs}" _search_dirs)
      string(REGEX REPLACE "^programs: =.*libraries: =(.+)$" "\\1" _lib_path "${_search_dirs}")
      if(_lib_path)
        file(TO_CMAKE_PATH "${_lib_path}" _lib_path)
        cmake_path(APPEND _lib_path "lib" "windows")
        if(EXISTS "${_lib_path}")
          file(TO_NATIVE_PATH "${_lib_path}" _lib_path)
          list(APPEND _link_options "/LIBPATH:${_lib_path}")
        endif()
      endif()
    endif()
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    get_filename_component(_compiler_path "${CMAKE_CXX_COMPILER}" PATH)
    string(REGEX MATCH "^[0-9]+" _gcc_version "${CMAKE_CXX_COMPILER_VERSION}")

    find_program(
      GCOV_PATH
      NAMES "gcov" "gcov-${_gcc_version}"
      HINTS ${_compiler_path}
    )
    if(NOT GCOV_PATH)
      message(SEND_ERROR "Missing: GCOV_PATH")
    endif()

    list(APPEND _compile_options
      "-fprofile-arcs"
      "-ftest-coverage"
    )
    list(APPEND _link_options
      "-fprofile-arcs"
      "-ftest-coverage"
    )
  else()
    message(FATAL_ERROR "Code coverage is only support for clang and gcc.\n"
                        "Please choose another compiler or disable code coverage.")
  endif()
  
  add_library(code_coverage_options INTERFACE)
  target_compile_options(code_coverage_options INTERFACE ${_compile_options})
  target_link_options(code_coverage_options INTERFACE ${_link_options})

  set_property(GLOBAL PROPERTY "_CodeCoverage_enabled" TRUE)
  set_property(GLOBAL PROPERTY "_CodeCoverage_targets" "")
endfunction()

function(code_coverage_add_target TARGET)
  get_property(_coverage_enabled GLOBAL PROPERTY "_CodeCoverage_enabled")
  if(NOT _coverage_enabled)
    return()
  endif()

  set_property(GLOBAL APPEND PROPERTY "_CodeCoverage_targets" "${TARGET}")

  target_link_libraries(${TARGET} PRIVATE code_coverage_options)
endfunction()

function(code_coverage_setup_discovered_tests TARGET)
  get_property(_coverage_enabled GLOBAL PROPERTY "_CodeCoverage_enabled")
  if(NOT _coverage_enabled)
    return()
  endif()
  
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(test_include_file "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_code_coverage_environment.cmake")
    file(GENERATE
      OUTPUT  "${test_include_file}"
      CONTENT "\
      foreach(test_name IN LISTS ${TARGET}_TESTS)\n\
        string(REPLACE \":\" \"_\" file_name \"\${test_name}\")\n\
        set_tests_properties(\${test_name} PROPERTIES ENVIRONMENT \"LLVM_PROFILE_FILE=code_coverage/\${file_name}.profraw\")\n\
      endforeach()\n"
    )
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/code_coverage") # TODO: this should be the working directory of the test
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} APPEND PROPERTY TEST_INCLUDE_FILES "${test_include_file}")
  endif()
endfunction()

function(code_coverage_report)
  cmake_parse_arguments(_args "" "EXCLUDE" "" ${ARGN})

  get_property(_coverage_enabled GLOBAL PROPERTY "_CodeCoverage_enabled")
  if(NOT _coverage_enabled)
    return()
  endif()

  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(_all_objects)
    get_property(_targets GLOBAL PROPERTY "_CodeCoverage_targets")
    foreach(target_name IN LISTS _targets)
      list(APPEND _all_objects
        -object "$<TARGET_FILE:${target_name}>"
      )
    endforeach()

    set(_data_dir "${CMAKE_BINARY_DIR}/tests/unit/code_coverage") # TODO: detect these

    file(WRITE "${CMAKE_BINARY_DIR}/find-profdata-files.cmake"
      "file(GLOB_RECURSE profdata_files\n"
      "  LIST_DIRECTORIES FALSE\n"
      "  \"${_data_dir}/*.profraw\"\n"
      ")\n"
      "set(_content)\n"
      "foreach(file IN LISTS profdata_files)\n"
      "  set(_content \"\${_content}\${file}\\n\")\n"
      "endforeach()\n"
      "file(WRITE \"${CMAKE_BINARY_DIR}/all-profdata-inputs.txt\"\n"
      "  \"\${_content}\"\n"
      ")\n"
    )

    add_custom_target(code_coverage_merge
      COMMAND ${CMAKE_COMMAND} -P find-profdata-files.cmake
      COMMAND ${LLVM_PROFDATA_PATH} merge -sparse --output=merged.profdata --input-files=all-profdata-inputs.txt
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Merge code coverage profile data..."
    )

    set(additional_report_args)
    if(_args_EXCLUDE)
      list(APPEND additional_report_args "--ignore-filename-regex=\"${_args_EXCLUDE}\"")
    endif()

    add_custom_target(code_coverage_report
      DEPENDS code_coverage_merge
      COMMAND ${LLVM_COV_PATH} report ${additional_report_args} --instr-profile=merged.profdata ${_all_objects}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    add_custom_target(code_coverage_export_lcov
      DEPENDS code_coverage_merge
      COMMAND ${LLVM_COV_PATH} export ${additional_report_args} --format=lcov --instr-profile=merged.profdata ${_all_objects} > coverage.info
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Export LCOV code coverage data..."
    )
  else()
    add_custom_target(coverage_report)
    add_custom_target(coverage_export)
  endif()
endfunction()
