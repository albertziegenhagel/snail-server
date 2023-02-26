
add_library(compile_options INTERFACE)

if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  endif()

  target_compile_options(compile_options INTERFACE
    "/W4"
    "/permissive-"
  )
elseif(CMAKE_COMPILER_IS_GNUCC)
  target_compile_options(compile_options INTERFACE
    "-Wall"
    "-Wextra"
    "-pedantic"
  )
endif()

target_compile_features(compile_options INTERFACE cxx_std_23)