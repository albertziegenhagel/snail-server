# Find the Npm executable
#
# Npm_FOUND      - Whether npm has been found
# Npm_EXECUTABLE - Path to the npm executable if found
# Npm_VERSION    - Version of the npm executable that has been found

set(_old_Npm_EXECUTABLE "${Npm_EXECUTABLE}")
find_program(Npm_EXECUTABLE
  NAMES
    npm
  HINTS
    $ENV{NPM_DIR}
  PATH_SUFFIXES
    bin
  DOC
    "Npm.js executable"
)
mark_as_advanced(Npm_EXECUTABLE)

if(WIN32 AND Npm_EXECUTABLE)
  set(Npm_EXECUTABLE "${Npm_EXECUTABLE}.cmd")
  set(_old_Npm_EXECUTABLE "${_old_Npm_EXECUTABLE}.cmd")
endif()

if(Npm_EXECUTABLE)
  if(Npm_VERSION_internal_saved AND "${Npm_EXECUTABLE}" STREQUAL "${_old_Npm_EXECUTABLE}")
    set(Npm_VERSION "${Npm_VERSION_internal_saved}")
  else()
    execute_process(
      COMMAND
        "${Npm_EXECUTABLE}" --version
      OUTPUT_VARIABLE
        _Npm_version
      RESULT_VARIABLE
        _Npm_version_result
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(_Npm_version_result)
      message(WARNING "Unable to determine Npm version: ${_Npm_version_result}")
    else()
      string(REGEX MATCH [[[0-9]+\.[0-9]+\.[0-9]+]] Npm_VERSION "${_Npm_version}")
      set(Npm_VERSION_internal_saved "${Npm_VERSION}" CACHE INTERNAL "")
    endif()
    unset(_Npm_version)
    unset(_Npm_version_result)
  endif()
endif()
unset(_old_Npm_EXECUTABLE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Npm
  REQUIRED_VARS
    Npm_EXECUTABLE
  VERSION_VAR
    Npm_VERSION
)
