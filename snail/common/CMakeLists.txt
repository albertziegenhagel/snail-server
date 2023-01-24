
add_library(common)

target_sources(common PRIVATE
  unicode.cpp
)

target_link_libraries(common compile_options)
# target_compile_definitions(common PRIVATE
#   "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING"
# )

target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR})
