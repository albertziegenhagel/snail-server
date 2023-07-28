
add_executable(snailserver)

target_sources(snailserver
  PRIVATE
    main.cpp

    storage/storage.cpp

    requests/requests.cpp
)

target_link_libraries(snailserver
  PRIVATE
    compile_options
  PUBLIC
    jsonrpc
    analysis
)

set_target_properties(snailserver PROPERTIES OUTPUT_NAME "snail-server")

code_coverage_add_target(snailserver)

install(
  TARGETS
    snailserver
  COMPONENT
    server
  RUNTIME DESTINATION bin
)
