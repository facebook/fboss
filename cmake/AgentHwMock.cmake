# CMake to build libraries and binaries in fboss/agent/hw/sai/hw_test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(pkt
  fboss/agent/hw/mock/MockRxPacket.cpp
  fboss/agent/hw/mock/MockTxPacket.cpp
)

target_link_libraries(pkt
  core
  Folly::folly
)
