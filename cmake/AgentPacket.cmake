# CMake to build libraries and binaries in fboss/agent/packet

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(packet
  fboss/agent/packet/ArpHdr.cpp
  fboss/agent/packet/DHCPv4Packet.cpp
  fboss/agent/packet/DHCPv6Packet.cpp
  fboss/agent/packet/EthHdr.cpp
  fboss/agent/packet/ICMPHdr.cpp
  fboss/agent/packet/IPv4Hdr.cpp
  fboss/agent/packet/IPv6Hdr.cpp
  fboss/agent/packet/LlcHdr.cpp
  fboss/agent/packet/MPLSHdr.cpp
  fboss/agent/packet/NDP.cpp
  fboss/agent/packet/NDPRouterAdvertisement.cpp
  fboss/agent/packet/PktUtil.cpp
  fboss/agent/packet/PTPHeader.cpp
  fboss/agent/packet/TCPHeader.cpp
  fboss/agent/packet/UDPHeader.cpp
)

target_link_libraries(packet
  pktutil
  stats
  utils
  Folly::folly
)

add_library(sflow_structs
  fboss/agent/packet/SflowStructs.cpp
)

target_link_libraries(sflow_structs
  Folly::folly
)

add_library(pktutil
  fboss/agent/packet/PktUtil.cpp
)

target_link_libraries(pktutil
  error
  Folly::folly
)

add_library(packet_factory
  fboss/agent/packet/PktFactory.cpp
)

target_link_libraries(packet_factory
  packet
  ctrl_cpp2
  switch_config_cpp2
  Folly::folly
)
