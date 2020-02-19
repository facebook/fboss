# CMake to build libraries and binaries in fboss/agent/hw/sai/switch

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sai_switch
  fboss/agent/hw/sai/switch/ConcurrentIndices.cpp
  fboss/agent/hw/sai/switch/SaiBridgeManager.cpp
  fboss/agent/hw/sai/switch/SaiFdbManager.cpp
  fboss/agent/hw/sai/switch/SaiHashManager.cpp
  fboss/agent/hw/sai/switch/SaiHostifManager.cpp
  fboss/agent/hw/sai/switch/SaiManagerTable.cpp
  fboss/agent/hw/sai/switch/SaiNeighborManager.cpp
  fboss/agent/hw/sai/switch/SaiNextHopManager.cpp
  fboss/agent/hw/sai/switch/SaiNextHopGroupManager.cpp
  fboss/agent/hw/sai/switch/SaiPortManager.cpp
  fboss/agent/hw/sai/switch/SaiQueueManager.cpp
  fboss/agent/hw/sai/switch/SaiRouteManager.cpp
  fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.cpp
  fboss/agent/hw/sai/switch/SaiRxPacket.cpp
  fboss/agent/hw/sai/switch/SaiSchedulerManager.cpp
  fboss/agent/hw/sai/switch/SaiSwitch.cpp
  fboss/agent/hw/sai/switch/SaiSwitchManager.cpp
  fboss/agent/hw/sai/switch/SaiTxPacket.cpp
  fboss/agent/hw/sai/switch/SaiVlanManager.cpp
  fboss/agent/hw/sai/switch/SaiVirtualRouterManager.cpp
)

target_link_libraries(sai_switch
  core
  # TODO (skhare)
  # Don't unconditionally link with fake_sai. Instead, allow linking with sai
  # implementation (--unresolved-symbols = <ignore..>).
  fake_sai
  hw_port_fb303_stats
  sai_api
  sai_store
  ref_map
  Folly::folly
)

set_target_properties(sai_switch PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

add_library(thrift_handler
  fboss/agent/hw/sai/switch/SaiHandler.cpp
)

target_link_libraries(thrift_handler
  diag_shell
  handler
  sai_ctrl_cpp2
)
