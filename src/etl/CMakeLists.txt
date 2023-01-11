
add_library(etlreader)

target_sources(etlreader PRIVATE
  etlreader.cpp
  etlfile.cpp

  parser/utility.cpp
  parser/buffer.cpp
  parser/trace.cpp
  parser/trace_headers/system_trace.cpp
  parser/trace_headers/compact_trace.cpp
  parser/trace_headers/perfinfo_trace.cpp
  parser/trace_headers/event_header_trace.cpp
  parser/trace_headers/full_header_trace.cpp
  parser/trace_headers/instance_trace.cpp
  
  parser/records/kernel/header.cpp
  parser/records/kernel/image.cpp
  parser/records/kernel/process.cpp
  parser/records/kernel/thread.cpp
  parser/records/kernel/perfinfo.cpp
  parser/records/kernel/stackwalk.cpp
  
)

target_link_libraries(etlreader compile_options)

target_link_libraries(etlreader binstream)

target_link_libraries(etlreader DiaSDK::DiaSDK)

target_include_directories(etlreader PUBLIC ..)
