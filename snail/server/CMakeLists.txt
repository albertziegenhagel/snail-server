
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
