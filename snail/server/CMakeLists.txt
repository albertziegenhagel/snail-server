
add_executable(snailserver)

target_sources(snailserver
  PRIVATE
    main.cpp

    snail_server.cpp

    detail/document_task_scheduler.cpp
    detail/storage.cpp
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
if(SNAIL_INSTALL_PDB)
  install(
    FILES
      "$<TARGET_PDB_FILE:snailserver>"
    COMPONENT
      server
    DESTINATION "bin"
    OPTIONAL
  )
endif()
