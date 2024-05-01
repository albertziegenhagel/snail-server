cmake_minimum_required(VERSION 3.20)

project(snail_server_apps)

include(ExternalProject)

set(apps
  "inner"
  "ordered"
)
set(build_types
  "Debug"
  "Release"
)

string(TOLOWER "${CMAKE_SYSTEM_NAME}" system)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  find_program(VS_DIAGNOSTICS_EXECUTABLE
    VSDiagnostics
    HINTS
      "$ENV{VSINSTALLDIR}\\Team Tools\\DiagnosticsHub\\Collector"
  )
  set(XPERF_EXECUTABLE "xperf")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_program(PERF_EXECUTABLE
    perf
  )
endif()

add_custom_target(record)
add_custom_target(install-apps)
add_custom_target(install-records)

foreach(app IN LISTS apps)
  foreach(build_type IN LISTS build_types)  
    string(TOLOWER "${build_type}" build_type_short)
    string(SUBSTRING "${build_type_short}" 0 3 build_type_short)

    set(app_project_name "${app}_${build_type_short}")

    set(project_prefix     "${app}/${build_type}")
    set(project_binary_dir "${project_prefix}/build")
    set(executable_path    "${project_binary_dir}/${app}${CMAKE_EXECUTABLE_SUFFIX}")

    ExternalProject_Add(${app_project_name}
      PREFIX           "${project_prefix}"
      BINARY_DIR       "${project_binary_dir}"
      SOURCE_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/${app}"
      CMAKE_ARGS       "-DCMAKE_BUILD_TYPE=${build_type}"
      INSTALL_COMMAND  ""
      BUILD_BYPRODUCTS "${executable_path}"
    )
    ExternalProject_Get_Property(${app_project_name} BINARY_DIR)

    set(record_working_dir "${CMAKE_CURRENT_BINARY_DIR}/records/${app}/${build_type}")
    file(MAKE_DIRECTORY "${record_working_dir}")

    set(destination_path "${CMAKE_CURRENT_SOURCE_DIR}/${app}/dist/${system}/${build_type_short}")
    file(MAKE_DIRECTORY "${destination_path}/bin")
    file(MAKE_DIRECTORY "${destination_path}/record")

    # Copy the executable (and PDB if necessary) to the source tree
    set(install_app_commands
      COMMAND "${CMAKE_COMMAND}" -E copy
        "${executable_path}"
        "${destination_path}/bin"
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND build_type STREQUAL Debug)
      list(APPEND install_app_commands
        COMMAND "${CMAKE_COMMAND}" -E copy
          "${BINARY_DIR}/${app}.pdb"
          "${destination_path}/bin"
      )
    endif()
    add_custom_target(install-${app_project_name}
      ${install_app_commands}
      DEPENDS ${app_project_name}
    )
    add_dependencies(install-apps install-${app_project_name})

    # Record a performance trace (diagsession on Windows, perf.data on Linux)
    set(native_abs_executable_path "${CMAKE_CURRENT_BINARY_DIR}/${executable_path}")
    cmake_path(NATIVE_PATH native_abs_executable_path native_abs_executable_path)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
      if(app STREQUAL "ordered")
        set(record_outputs
          "${record_working_dir}/${app}.etl"
          "${record_working_dir}/${app}_merged.etl"
        )
        set(record_command
          powershell -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/record-win-xperf.ps1" "${XPERF_EXECUTABLE}" "${native_abs_executable_path}" "${app}"
        )
        message("${record_command}")
      else()
        set(record_outputs "${record_working_dir}/${app}.diagsession")
        set(namespace_guid "DE824224-CA40-4B54-AB87-C38A7A3E53AF") # Randomly generated
        string(UUID session_id NAMESPACE "${namespace_guid}" NAME "${app_project_name}" TYPE SHA1 UPPER)
        set(record_command
          powershell -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/record-win.ps1" "${VS_DIAGNOSTICS_EXECUTABLE}" "${native_abs_executable_path}" "${session_id}" "${app}"
        )
      endif()
      add_custom_command(
        OUTPUT ${record_outputs}
        COMMAND ${record_command}
        MAIN_DEPENDENCY   "${executable_path}"
        WORKING_DIRECTORY "${record_working_dir}"
      )
    else()
      if(app STREQUAL "ordered")
        set(record_events
          -e "cycles,branch-misses,cache-references,cache-misses"
          -F 5000
          )
      else()
        set(record_events)
      endif()
      set(record_outputs "${record_working_dir}/${app}-perf.data")
      add_custom_command(
        OUTPUT ${record_outputs}
        COMMAND "${PERF_EXECUTABLE}" record -g ${record_events} -o "${app}-perf.data" "${native_abs_executable_path}"
        MAIN_DEPENDENCY   "${executable_path}"
        WORKING_DIRECTORY "${record_working_dir}"
      )
    endif()

    add_custom_target(record-${app_project_name}
      DEPENDS ${record_outputs}
    )
    add_dependencies(record record-${app_project_name})

    set(install_commands)
    foreach(record_output IN LISTS record_outputs)
      list(APPEND install_commands
        COMMAND "${CMAKE_COMMAND}" -E copy
          "${record_output}"
          "${destination_path}/record"
      )
    endforeach()
    add_custom_target(install-record-${app_project_name}
      ${install_commands} 
      DEPENDS ${record_outputs}
    )

    add_dependencies(install-records install-record-${app_project_name})
  endforeach()
endforeach()
