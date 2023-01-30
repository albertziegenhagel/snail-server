
add_executable(snailserver)

target_sources(snailserver PRIVATE
  main.cpp
)

target_link_libraries(snailserver compile_options jsonrpc analysis)
