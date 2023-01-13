
add_library(compile_options INTERFACE)

option(SNAIL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(MSVC)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  endif()

  target_compile_options(compile_options INTERFACE
    "/W4"
    "/permissive-"
    "$<$<CONFIG:Release>:/arch:AVX2>"
  )
  if(SNAIL_WARNINGS_AS_ERRORS)
    target_compile_options(compile_options INTERFACE
        "/WX"
    )
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC)
  target_compile_options(compile_options INTERFACE
    "-Wall"
    "-Wextra"
    "-pedantic"
    "$<$<CONFIG:Release>:-march=native"
  )
  if(SNAIL_WARNINGS_AS_ERRORS)
    target_compile_options(compile_options INTERFACE
        "-Werror"
    )
  endif()
endif()

target_compile_features(compile_options INTERFACE cxx_std_23)