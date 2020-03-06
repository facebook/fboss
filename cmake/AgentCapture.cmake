# CMake to build libraries and binaries in fboss/agent/capture

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(capture
  fboss/agent/capture/PcapFile.cpp
  fboss/agent/capture/PcapPkt.cpp
  fboss/agent/capture/PcapQueue.cpp
  fboss/agent/capture/PcapWriter.cpp
  fboss/agent/capture/PktCapture.cpp
  fboss/agent/capture/PktCaptureManager.cpp
)

target_link_libraries(capture
  packet
  Folly::folly
)
