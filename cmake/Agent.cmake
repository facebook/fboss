# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(setup_thrift
  fboss/agent/SetupThrift.cpp
  fboss/agent/oss/SetupThrift.cpp
)

target_link_libraries(setup_thrift
  Folly::folly
  FBThrift::thriftcpp2
)

add_library(fboss_init
  fboss/agent/oss/FbossInit.cpp
)

target_link_libraries(fboss_init
  Folly::folly
)

add_library(address_utils
  fboss/agent/AddressUtil.h
)

target_link_libraries(address_utils
  Folly::folly
)

add_library(utils
  fboss/agent/AlpmUtils.cpp
  fboss/agent/Utils.cpp
  fboss/agent/oss/Utils.cpp
)

target_link_libraries(utils
  error
  ctrl_cpp2
  switch_state_cpp2
  Folly::folly
)

add_library(stats
  fboss/agent/AggregatePortStats.cpp
  fboss/agent/PortStats.cpp
  fboss/agent/SwitchStats.cpp
  fboss/agent/oss/AggregatePortStats.cpp
)

target_link_libraries(stats
  fboss_types
  state
  Folly::folly
)

add_library(fboss_types
  fboss/agent/types.cpp
)

target_link_libraries(fboss_types
  switch_config_cpp2
)

add_library(core
  fboss/agent/ApplyThriftConfig.cpp
  fboss/agent/ArpCache.cpp
  fboss/agent/ArpHandler.cpp
  fboss/agent/DHCPv4Handler.cpp
  fboss/agent/DHCPv6Handler.cpp
  fboss/agent/HwSwitch.cpp
  fboss/agent/IPHeaderV4.cpp
  fboss/agent/IPv4Handler.cpp
  fboss/agent/IPv6Handler.cpp
  fboss/agent/L2Entry.cpp
  fboss/agent/LacpController.cpp
  fboss/agent/LacpMachines.cpp
  fboss/agent/LacpTypes.cpp
  fboss/agent/LinkAggregationManager.cpp
  fboss/agent/LldpManager.cpp
  fboss/agent/LoadBalancerConfigApplier.cpp
  fboss/agent/LookupClassUpdater.cpp
  fboss/agent/LookupClassRouteUpdater.cpp
  fboss/agent/MacTableManager.cpp
  fboss/agent/MacTableUtils.cpp
  fboss/agent/MirrorManager.cpp
  fboss/agent/MirrorManagerImpl.cpp
  fboss/agent/NdpCache.cpp
  fboss/agent/NeighborUpdater.cpp
  fboss/agent/NeighborUpdaterImpl.cpp
  fboss/agent/PortUpdateHandler.cpp
  fboss/agent/ResolvedNexthopMonitor.cpp
  fboss/agent/ResolvedNexthopProbe.cpp
  fboss/agent/ResolvedNexthopProbeScheduler.cpp
  fboss/agent/RestartTimeTracker.cpp
  fboss/agent/RouteUpdateLogger.cpp
  fboss/agent/RouteUpdateLoggingPrefixTracker.cpp
  fboss/agent/StandaloneRibConversions.cpp
  fboss/agent/StaticL2ForNeighborObserver.cpp
  fboss/agent/StaticL2ForNeighborUpdater.cpp
  fboss/agent/StaticL2ForNeighborSwSwitchUpdater.cpp
  fboss/agent/SwSwitch.cpp
  fboss/agent/ThreadHeartbeat.cpp
  fboss/agent/TunIntf.cpp
  fboss/agent/TunManager.cpp
  fboss/agent/ndp/IPv6RouteAdvertiser.cpp
  fboss/agent/oss/RouteUpdateLogger.cpp
  fboss/agent/oss/SwSwitch.cpp
)

target_link_libraries(core
  agent_config_cpp2
  stats
  utils
  fb303::fb303
  capture
  hardware_stats_cpp2
  switch_asics
  ctrl_cpp2
  fboss_cpp2
  lldp
  packet
  product_info
  platform_base
  fib_updater
  network_to_route_map
  standalone_rib
  state
  state_utils
  exponential_back_off
  fboss_config_utils
  phy_cpp2
  transceiver_cpp2
  alert_logger
  Folly::folly
  normalizer
  ${IPROUTE2}
  ${NETLINK3}
  ${NETLINKROUTE3}
)

add_library(error
  fboss/agent/FbossError.h
)

target_link_libraries(error
  ctrl_cpp2
  Folly::folly
)

add_library(handler
  fboss/agent/ThriftHandler.cpp
)

target_link_libraries(handler
  core
  pkt
  fb303::fb303
  ctrl_cpp2
  log_thrift_call
  Folly::folly
)

target_link_libraries(fboss_types
  switch_config_cpp2
  Folly::folly
)

add_library(fboss_error
  fboss/agent/FbossError.h
  fboss/agent/SysError.h
)

target_link_libraries(fboss_error
  fboss_cpp2
  fboss_types
  Folly::folly
)

add_library(platform_base
  fboss/agent/AgentConfig.cpp
  fboss/agent/Platform.cpp
  fboss/agent/PlatformPort.cpp
)

target_link_libraries(platform_base
  agent_config_cpp2
  ctrl_cpp2
  error
  fboss_types
  Folly::folly
  platform_mapping
)

add_library(hw_switch
  fboss/agent/HwSwitch.cpp
)

target_link_libraries(hw_switch
  fboss_types
  ctrl_cpp2
  fboss_cpp2
  Folly::folly
  platform_base
  hw_switch_stats
)

add_library(async_logger
  fboss/agent/AsyncLogger.cpp
)

target_link_libraries(async_logger
  fboss_error
  Folly::folly
)
