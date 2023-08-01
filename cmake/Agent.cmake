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
  fboss/agent/CommonInit.cpp
  fboss/agent/oss/CommonInit.cpp
  fboss/agent/oss/Main.cpp
)

target_link_libraries(main
  fboss_common_init
# base
  fboss_init
  Folly::folly
)

add_library(monolithic_switch_handler
  fboss/agent/single/MonolithicHwSwitchHandler.cpp
)

target_link_libraries(monolithic_switch_handler
  hw_switch
  packet
  platform_base
  hw_switch_fb303_stats
  switch_asics
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

add_library(switchinfo_utils
  fboss/agent/SwitchInfoUtils.cpp
)

target_link_libraries(switchinfo_utils
  Folly::folly
  switch_config_cpp2
  switch_state_cpp2
  switch_asics
  ctrl_cpp2
  fboss_types
)

target_link_libraries(address_utils
  network_address_cpp2
  Folly::folly
)

add_library(utils
  fboss/agent/AlpmUtils.cpp
  fboss/agent/LabelFibUtils.cpp
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
  fboss/agent/HwSwitchSyncer.cpp
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
  fboss/agent/MultiHwSwitchSyncer.cpp
  fboss/agent/MultiSwitchPacketStreamMap.cpp
  fboss/agent/NdpCache.cpp
  fboss/agent/NeighborUpdater.cpp
  fboss/agent/NeighborUpdaterImpl.cpp
  fboss/agent/NeighborUpdaterNoopImpl.cpp
  fboss/agent/PortUpdateHandler.cpp
  fboss/agent/ResolvedNexthopMonitor.cpp
  fboss/agent/ResolvedNexthopProbe.cpp
  fboss/agent/ResolvedNexthopProbeScheduler.cpp
  fboss/agent/RouteUpdateLogger.cpp
  fboss/agent/RouteUpdateLoggingPrefixTracker.cpp
  fboss/agent/StaticL2ForNeighborObserver.cpp
  fboss/agent/StaticL2ForNeighborUpdater.cpp
  fboss/agent/StaticL2ForNeighborSwSwitchUpdater.cpp
  fboss/agent/SwitchInfoTable.cpp
  fboss/agent/SwitchStatsObserver.cpp
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
  switchinfo_utils
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
  multiswitch_ctrl_cpp2
  packet
  product_info
  platform_base
  restart_time_tracker
  route_update_wrapper
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
  fboss/agent/Platform.cpp
  fboss/agent/PlatformPort.cpp
)

target_link_libraries(platform_base
  agent_config_cpp2
  ctrl_cpp2
  error
  fboss_types
  Folly::folly
  load_agent_config
  platform_mapping
  switchid_scope_resolver
  switchinfo_utils
)

add_library(hw_switch
  fboss/agent/HwSwitch.cpp
  fboss/agent/HwSwitchRouteUpdateWrapper.cpp
  fboss/agent/oss/HwSwitch.cpp
)

target_link_libraries(hw_switch
  fboss_types
  ctrl_cpp2
  fboss_cpp2
  Folly::folly
  platform_base
  route_update_wrapper
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
  fboss/fsdb/common/Utils.cpp
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
  fboss_error
  hwswitch_matcher
  state
)

add_library(hwagent
  fboss/agent/HwAgent.cpp
)

target_link_libraries(hwagent
  fboss_init
  hw_switch
  platform_base
  switch_asics
)

add_library(split_agent_hwswitch_callback_handler
  fboss/agent/mnpu/SplitAgentHwSwitchCallbackHandler.cpp
)

target_link_libraries(split_agent_hwswitch_callback_handler
  Folly::folly
  fboss_types
  state
)

add_library(hwagent-main
  fboss/agent/HwAgentMain.cpp
  fboss/agent/oss/Main.cpp
)

target_link_libraries(hwagent-main
  fboss_common_init
  split_agent_hwswitch_callback_handler
  platform_base
  fboss_common_cpp2
  restart_time_tracker
  setup_thrift
  split_agent_thrift_syncer
  Folly::folly
)

add_library(restart_time_tracker
  fboss/agent/RestartTimeTracker.cpp
)

target_link_libraries(restart_time_tracker
  utils
  fb303::fb303
  Folly::folly
)

add_library(multiswitch_service
  fboss/agent/MultiSwitchThriftHandler.cpp
)

target_link_libraries(multiswitch_service
  core
  multiswitch_ctrl_cpp2
  state
  packet
)

add_library(route_update_wrapper
  fboss/agent/RouteUpdateWrapper.cpp
)

target_link_libraries(route_update_wrapper
  address_utils
  fboss_types
  switch_config_cpp2
  switchid_scope_resolver
  ctrl_cpp2
  fib_updater
  standalone_rib
  state
  Folly::folly
)

add_library(split_agent_thrift_syncer
  fboss/agent/mnpu/SplitAgentThriftSyncer.cpp
)

target_link_libraries(split_agent_thrift_syncer
  multiswitch_service
  Folly::folly
)

add_library(load_agent_config
    fboss/agent/AgentConfig.cpp
)

target_link_libraries(load_agent_config
  agent_config_cpp2
  fboss_error
  switch_config_cpp2
  Folly::folly
)


add_library(fboss_common_init
  fboss/agent/CommonInit.cpp
  fboss/agent/oss/CommonInit.cpp
)

target_link_libraries(fboss_common_init
  fboss_init
  load_agent_config
  Folly::folly
  fb303::fb303
)

add_library(monolithic_agent_initializer
  fboss/agent/single/MonolithicAgentInitializer.cpp
)

target_link_libraries(monolithic_agent_initializer
  core
  fboss_common_init
  handler
  hw_switch
  load_agent_config
  monolithic_switch_handler
  setup_thrift
  switch_asics
  utils
  Folly::folly
)


add_library(split_agent_initializer
  fboss/agent/mnpu/SwAgentInitializer.cpp
)

target_link_libraries(split_agent_initializer
  Folly::folly
)

add_executable(fboss_sw_agent
  fboss/agent/FbossSwAgent.cpp
)

  target_link_libraries(fboss_sw_agent
    main
    split_agent_initializer
  )
