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
  fboss_common_cpp2
)

# TODO (rsunkad) re-enable this library for use with linking with libmain
#add_library(base INTERFACE)

#target_link_libraries(base
#  core
#  handler
#  Folly::folly
#)

add_library(main
  fboss/agent/Main.cpp
  fboss/agent/oss/Main.cpp
)

target_link_libraries(main
  core
  handler
# base
  fboss_init
  setup_thrift
  Folly::folly
  qsfp_cpp2
  qsfp_service_client
)

add_library(async_packet_transport
  fboss/agent/AsyncPacketTransport.h
)

target_link_libraries(async_packet_transport
  Folly::folly
)

add_library(address_utils
  fboss/agent/AddressUtil.h
)

target_link_libraries(address_utils
  network_address_cpp2
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
  state
  Folly::folly
)

add_library(stats
  fboss/agent/AggregatePortStats.cpp
  fboss/agent/InterfaceStats.cpp
  fboss/agent/PortStats.cpp
  fboss/agent/SwitchStats.cpp
  fboss/agent/oss/AggregatePortStats.cpp
)

target_link_libraries(stats
  fboss_types
  agent_stats_cpp2
  state
  Folly::folly
)

add_library(fboss_types
  fboss/agent/types.cpp
  fboss/agent/PortDescriptorTemplate.cpp
)

target_link_libraries(fboss_types
  switch_config_cpp2
  Folly::folly
)

add_library(core
  fboss/agent/AclNexthopHandler.cpp
  fboss/agent/ApplyThriftConfig.cpp
  fboss/agent/ArpCache.cpp
  fboss/agent/ArpHandler.cpp
  fboss/agent/DHCPv4Handler.cpp
  fboss/agent/DHCPv6Handler.cpp
  fboss/agent/DsfSubscriber.cpp
  fboss/agent/FabricReachabilityManager.cpp
  fboss/agent/EncapIndexAllocator.cpp
  fboss/agent/FibHelpers.cpp
  fboss/agent/HwAsicTable.cpp
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
  fboss/agent/MKAServicePorts.cpp
  fboss/agent/MKAServiceManager.cpp
  fboss/agent/MacTableManager.cpp
  fboss/agent/MacTableUtils.cpp
  fboss/agent/MirrorManager.cpp
  fboss/agent/MirrorManagerImpl.cpp
  fboss/agent/MPLSHandler.cpp
  fboss/agent/NdpCache.cpp
  fboss/agent/NeighborUpdater.cpp
  fboss/agent/NeighborUpdaterImpl.cpp
  fboss/agent/NeighborUpdaterNoopImpl.cpp
  fboss/agent/PortUpdateHandler.cpp
  fboss/agent/ResolvedNexthopMonitor.cpp
  fboss/agent/ResolvedNexthopProbe.cpp
  fboss/agent/ResolvedNexthopProbeScheduler.cpp
  fboss/agent/RestartTimeTracker.cpp
  fboss/agent/RouteUpdateLogger.cpp
  fboss/agent/RouteUpdateLoggingPrefixTracker.cpp
  fboss/agent/RouteUpdateWrapper.cpp
  fboss/agent/StaticL2ForNeighborObserver.cpp
  fboss/agent/StaticL2ForNeighborUpdater.cpp
  fboss/agent/StaticL2ForNeighborSwSwitchUpdater.cpp
  fboss/agent/SwitchInfoTable.cpp
  fboss/agent/SwSwitch.cpp
  fboss/agent/SwSwitchRouteUpdateWrapper.cpp
  fboss/agent/TeFlowNexthopHandler.cpp
  fboss/agent/TunIntf.cpp
  fboss/agent/TunManager.cpp
  fboss/agent/ndp/IPv6RouteAdvertiser.cpp
  fboss/agent/oss/HwSwitch.cpp
  fboss/agent/oss/PacketLogger.cpp
  fboss/agent/oss/RouteUpdateLogger.cpp
  fboss/agent/oss/SwSwitch.cpp
  fboss/agent/oss/DsfSubscriber.cpp
  fboss/agent/oss/FsdbSyncer.cpp
)

target_link_libraries(core
  agent_config_cpp2
  stats
  utils
  fb303::fb303
  capture
  diag_cmd_filter
  hardware_stats_cpp2
  hw_switch_fb303_stats
  switch_asics
  switchid_scope_resolver
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
  phy_utils
  snapshot_manager
  transceiver_cpp2
  alert_logger
  Folly::folly
  normalizer
  bidirectional_packet_stream
  fsdb_stream_client
  fsdb_pub_sub
  fsdb_flags
  ${IPROUTE2}
  ${NETLINK3}
  ${NETLINKROUTE3}
  thread_heartbeat
  platform_mapping_utils
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
  fboss/agent/oss/HwSwitch.cpp
)

target_link_libraries(hw_switch
  fboss_types
  ctrl_cpp2
  fboss_cpp2
  Folly::folly
  platform_base
  hw_switch_fb303_stats
)

add_library(async_logger
  fboss/agent/AsyncLogger.cpp
)

target_link_libraries(async_logger
  fboss_error
  fb303::fb303
  Folly::folly
)

add_library(sflow_shim_utils
  fboss/agent/SflowShimUtils.cpp
)

target_link_libraries(sflow_shim_utils
  Folly::folly
)


add_library(fsdb_helper
  fboss/agent/oss/FsdbHelper.cpp
)

target_link_libraries(fsdb_helper
  fsdb_oper_cpp2
  state
)

add_library(hwswitch_matcher
  fboss/agent/HwSwitchMatcher.cpp
)

target_link_libraries(hwswitch_matcher
  fboss_error
  fboss_types
)

add_library(switchid_scope_resolver
  fboss/agent/SwitchIdScopeResolver.cpp
)

target_link_libraries(switchid_scope_resolver
  fboss_types
)
