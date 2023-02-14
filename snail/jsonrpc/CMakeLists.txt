
add_library(jsonrpc)

target_sources(jsonrpc
  PRIVATE
    server.cpp
    errors.cpp

    jsonrpc_v2_protocol.cpp

    transport/message_connection.cpp
    transport/stream_message_reader.cpp
    transport/stream_message_writer.cpp
    
    stream/generic/streambuf_base.cpp
)

if(UNIX)
  target_sources(jsonrpc
    PRIVATE
      stream/unix/unix_domain_socket_streambuf.cpp
      stream/unix/unix_domain_socket_iostream.cpp
  )
endif()

if(WIN32)
  target_sources(jsonrpc
    PRIVATE
      stream/windows/pipe_streambuf.cpp
      stream/windows/pipe_iostream.cpp
  )
endif()

target_link_libraries(jsonrpc
  PRIVATE
    compile_options
  PUBLIC
    nlohmann_json::nlohmann_json
)

target_include_directories(jsonrpc PUBLIC ${CMAKE_SOURCE_DIR})
