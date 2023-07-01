cmake_minimum_required(VERSION 3.20)

project(snail_test_app_inner VERSION 0.1 LANGUAGES CXX)

add_executable(inner)

target_sources(inner
  PRIVATE
    main.cpp
)

target_compile_features(inner PRIVATE cxx_std_17)
