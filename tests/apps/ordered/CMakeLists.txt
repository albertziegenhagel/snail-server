cmake_minimum_required(VERSION 3.20)

project(snail_test_app_ordered VERSION 0.1 LANGUAGES CXX)

add_executable(ordered)

target_sources(ordered
  PRIVATE
    main.cpp
    impl.cpp
)

target_compile_features(ordered PRIVATE cxx_std_20)
