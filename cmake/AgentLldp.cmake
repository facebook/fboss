# CMake to build libraries and binaries in fboss/agent/hw/switch_asics

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake


add_fbthrift_cpp_library(
  lldp_structs_cpp2
  fboss/agent/lldp/lldp.thrift
  OPTIONS
    json
    reflection
)

add_library(lldp
  fboss/agent/lldp/LinkNeighbor.cpp
  fboss/agent/lldp/LinkNeighborDB.cpp
)

target_link_libraries(lldp
  fboss_types
  lldp_structs_cpp2
  Folly::folly
  thrift_cow_nodes
)
