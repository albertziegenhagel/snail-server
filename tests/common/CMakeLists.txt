
add_library(test_main STATIC
  main.cpp
  folders.cpp
)
target_link_libraries(test_main compile_options GTest::gtest)
target_include_directories(test_main
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
