
add_library(etlreader)

target_sources(etlreader PRIVATE
  etlreader.cpp
  etlfile.cpp
  dispatching_event_observer.cpp
)

target_link_libraries(etlreader compile_options)

target_link_libraries(etlreader DiaSDK::DiaSDK)

target_include_directories(etlreader PUBLIC ..)
