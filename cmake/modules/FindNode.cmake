# Find the Node executable
#
# Node_FOUND      - Whether node has been found
# Node_EXECUTABLE - Path to the node executable if found
# Node_VERSION    - Version of the node executable that has been found

find_program(Node_EXECUTABLE
  NAMES
    node
  HINTS
    $ENV{NODE_DIR}
  PATH_SUFFIXES
    bin
  DOC
    "Node.js executable"
)
mark_as_advanced(Node_EXECUTABLE)

if(Node_EXECUTABLE AND NOT Node_VERSION)
  execute_process(
    COMMAND
      "${Node_EXECUTABLE}" --version
    OUTPUT_VARIABLE
      _Node_version
    RESULT_VARIABLE
      _Node_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(_Node_version_result)
    message(WARNING "Unable to determine Node version: ${_Node_version_result}")
  endif()
  string(REGEX MATCH [[[0-9]+\.[0-9]+\.[0-9]+]] Node_VERSION "${_Node_version}")
  unset(_Node_version)
  unset(_Node_version_result)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Node
  REQUIRED_VARS
    Node_EXECUTABLE
  VERSION_VAR
    Node_VERSION
)
