
find_path(DiaSDK_INCLUDE_DIR
    dia2.h
    HINTS
        "$ENV{VSINSTALLDIR}DIA SDK/include"
        "${VSWHERE_LATEST}/DIA SDK/include"
    DOC "Path to DIA SDK header files"
)

if ((CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR ("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "x64"))
    set(_dia_library_platform_suffix "/amd64")
elseif ((CMAKE_GENERATOR_PLATFORM STREQUAL "ARM") OR ("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM"))
    set(_dia_library_platform_suffix "/arm")
elseif ((CMAKE_GENERATOR_PLATFORM MATCHES "ARM64.*") OR ("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" MATCHES "ARM64.*"))
    set(_dia_library_platform_suffix "/arm64")
else()
    set(_dia_library_platform_suffix "")
endif()

find_library(DiaSDK_LIBRARY
    NAMES diaguids.lib
    HINTS
        "$ENV{VSINSTALLDIR}DIA SDK/lib${_dia_library_platform_suffix}"
        "${VSWHERE_LATEST}/DIA SDK/lib${_dia_library_platform_suffix}"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DiaSDK
  FOUND_VAR
    DiaSDK_FOUND
  REQUIRED_VARS
    DiaSDK_LIBRARY
    DiaSDK_INCLUDE_DIR
#   VERSION_VAR
#     DiaSDK_VERSION
)


if(DiaSDK_FOUND AND NOT TARGET DiaSDK::DiaSDK)
  add_library(DiaSDK::DiaSDK UNKNOWN IMPORTED)
  set_target_properties(DiaSDK::DiaSDK PROPERTIES
    IMPORTED_LOCATION "${DiaSDK_LIBRARY}"
    # INTERFACE_COMPILE_OPTIONS "${DiaSDK_COMPILE_OPTIONS}"
    INTERFACE_INCLUDE_DIRECTORIES "${DiaSDK_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(DiaSDK_LIBRARY DiaSDK_INCLUDE_DIR)

if(DiaSDK_FOUND)
  set(DiaSDK_LIBRARIES ${DiaSDK_LIBRARY})
  set(DiaSDK_INCLUDE_DIRS ${DiaSDK_INCLUDE_DIR})
endif()
