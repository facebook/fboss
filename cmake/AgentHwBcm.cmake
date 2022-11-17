# CMake to build libraries and binaries in fboss/agent/hw/bcm

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(bcm
  fboss/agent/hw/bcm/BcmAclEntry.cpp
  fboss/agent/hw/bcm/BcmAclStat.cpp
  fboss/agent/hw/bcm/BcmAclTable.cpp
  fboss/agent/hw/bcm/BcmAddressFBConvertors.cpp
  fboss/agent/hw/bcm/BcmAPI.cpp
  fboss/agent/hw/bcm/BcmBstStatsMgr.cpp
  fboss/agent/hw/bcm/BcmFacebookAPI.cpp
  fboss/agent/hw/bcm/BcmControlPlane.cpp
  fboss/agent/hw/bcm/BcmControlPlaneQueueManager.cpp
  fboss/agent/hw/bcm/BcmCosQueueManager.cpp
  fboss/agent/hw/bcm/BcmCosQueueFBConvertors.cpp
  fboss/agent/hw/bcm/BcmCosQueueManagerUtils.cpp
  fboss/agent/hw/bcm/BcmCosManager.cpp
  fboss/agent/hw/bcm/BcmEcmpUtils.cpp
  fboss/agent/hw/bcm/BcmEgress.cpp
  fboss/agent/hw/bcm/BcmEgressManager.cpp
  fboss/agent/hw/bcm/BcmEgressQueueFlexCounter.cpp
  fboss/agent/hw/bcm/BcmExactMatchUtils.cpp
  fboss/agent/hw/bcm/BcmFieldProcessorFBConvertors.cpp
  fboss/agent/hw/bcm/BcmFieldProcessorUtils.cpp
  fboss/agent/hw/bcm/BcmFlexCounter.cpp
  fboss/agent/hw/bcm/BcmFwLoader.cpp
  fboss/agent/hw/bcm/BcmIngressFieldProcessorFlexCounter.cpp
  fboss/agent/hw/bcm/BcmHost.cpp
  fboss/agent/hw/bcm/BcmHostKey.cpp
  fboss/agent/hw/bcm/BcmIntf.cpp
  fboss/agent/hw/bcm/BcmLabelMap.cpp
  fboss/agent/hw/bcm/BcmLabelSwitchAction.cpp
  fboss/agent/hw/bcm/BcmLabelSwitchingUtils.cpp
  fboss/agent/hw/bcm/BcmLabeledEgress.cpp
  fboss/agent/hw/bcm/BcmLabeledTunnel.cpp
  fboss/agent/hw/bcm/BcmLabeledTunnelEgress.cpp
  fboss/agent/hw/bcm/BcmLogBuffer.cpp
  fboss/agent/hw/bcm/BcmMirror.cpp
  fboss/agent/hw/bcm/BcmMirrorTable.cpp
  fboss/agent/hw/bcm/BcmMirrorUtils.cpp
  fboss/agent/hw/bcm/BcmMultiPathNextHop.cpp
  fboss/agent/hw/bcm/BcmNextHop.cpp
  fboss/agent/hw/bcm/BcmPort.cpp
  fboss/agent/hw/bcm/BcmPortIngressBufferManager.cpp
  fboss/agent/hw/bcm/BcmPortUtils.cpp
  fboss/agent/hw/bcm/BcmPortDescriptor.cpp
  fboss/agent/hw/bcm/BcmPrbs.cpp
  fboss/agent/hw/bcm/BcmPtpTcMgr.cpp
  fboss/agent/hw/bcm/BcmQcmCollector.cpp
  fboss/agent/hw/bcm/BcmQcmManager.cpp
  fboss/agent/hw/bcm/BcmPortTable.cpp
  fboss/agent/hw/bcm/BcmPortGroup.cpp
  fboss/agent/hw/bcm/BcmPortQueueManager.cpp
  fboss/agent/hw/bcm/BcmPortResourceBuilder.cpp
  fboss/agent/hw/bcm/BcmPlatform.cpp
  fboss/agent/hw/bcm/BcmPlatformPort.cpp
  fboss/agent/hw/bcm/BcmQosPolicy.cpp
  fboss/agent/hw/bcm/BcmQosMap.cpp
  fboss/agent/hw/bcm/BcmQosMapEntry.cpp
  fboss/agent/hw/bcm/BcmQosPolicyTable.cpp
  fboss/agent/hw/bcm/BcmRoute.cpp
  fboss/agent/hw/bcm/BcmRouteCounter.cpp
  fboss/agent/hw/bcm/BcmRtag7LoadBalancer.cpp
  fboss/agent/hw/bcm/BcmRtag7Module.cpp
  fboss/agent/hw/bcm/BcmSflowExporter.cpp
  fboss/agent/hw/bcm/BcmRxPacket.cpp
  fboss/agent/hw/bcm/BcmStatUpdater.cpp
  fboss/agent/hw/bcm/BcmSwitch.cpp
  fboss/agent/hw/bcm/BcmSwitchEventCallback.cpp
  fboss/agent/hw/bcm/BcmSwitchEventUtils.cpp
  fboss/agent/hw/bcm/BcmTableStats.cpp
  fboss/agent/hw/bcm/BcmTeFlowEntry.cpp
  fboss/agent/hw/bcm/BcmTeFlowTable.cpp
  fboss/agent/hw/bcm/BcmTrunk.cpp
  fboss/agent/hw/bcm/BcmTrunkStats.cpp
  fboss/agent/hw/bcm/BcmTrunkTable.cpp
  fboss/agent/hw/bcm/BcmTxPacket.cpp
  fboss/agent/hw/bcm/BcmQosUtils.cpp
  fboss/agent/hw/bcm/BcmUnit.cpp
  fboss/agent/hw/bcm/BcmWarmBootCache.cpp
  fboss/agent/hw/bcm/BcmWarmBootHelper.cpp
  fboss/agent/hw/bcm/BcmWarmBootState.cpp
  fboss/agent/hw/bcm/PortAndEgressIdsMap.cpp
  fboss/agent/hw/bcm/BcmClassIDUtil.cpp
  fboss/agent/hw/bcm/BcmSwitchSettings.cpp
  fboss/agent/hw/bcm/BcmMacTable.cpp
  fboss/agent/hw/bcm/PacketTraceUtils.cpp
  fboss/agent/hw/bcm/RxUtils.cpp
  fboss/agent/hw/bcm/oss/BcmSwitch.cpp
  fboss/agent/hw/bcm/oss/BcmAPI.cpp
  fboss/agent/hw/bcm/oss/BcmFacebookAPI.cpp
  fboss/agent/hw/bcm/oss/BcmPort.cpp
  fboss/agent/hw/bcm/oss/BcmBstStatsMgr.cpp
  fboss/agent/hw/bcm/oss/BcmQcmCollector.cpp
  fboss/agent/hw/bcm/oss/BcmQcmManager.cpp
  fboss/agent/hw/bcm/oss/BcmUnit.cpp
  fboss/agent/hw/bcm/oss/BcmTableStats.cpp
)

target_link_libraries(bcm
  config
  sflow_cpp2
  hw_switch_warmboot_helper
  hw_switch_stats
  hw_trunk_counters
  hw_resource_stats_publisher
  bcm_types
  packettrace_cpp2
  buffer_stats
  handler
  core
  counter_utils
  Folly::folly
  ${OPENNSA}
  phy_utils
  bcm_yaml_config
)

set_target_properties(bcm PROPERTIES COMPILE_FLAGS
  "-DINCLUDE_L3 -DBCM_ESW_SUPPORT"
)

include_directories(
    ${LIBGMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

add_library(config
  fboss/agent/hw/bcm/BcmConfig.cpp
)

target_link_libraries(config
  bcm_config_cpp2
  error
  switch_config_cpp2
  Folly::folly
)

add_library(bcm_types
  fboss/agent/hw/bcm/BcmTypesImpl.cpp
)

target_link_libraries(bcm_types
  ${OPENNSA}
)

set_target_properties(bcm_types PROPERTIES COMPILE_FLAGS
  "-DINCLUDE_L3 -DBCM_ESW_SUPPORT"
)

add_library(bcm_mpls_utils
  fboss/agent/hw/bcm/BcmLabelSwitchingUtils.cpp
)

target_link_libraries(bcm_mpls_utils
  error
  mpls_cpp2
  label_forwarding_utils
  ${OPENNSA}
)

add_library(bcm_link_state_toggler
  fboss/agent/hw/bcm/tests/BcmLinkStateToggler.cpp
)

target_link_libraries(bcm_link_state_toggler
  bcm
  hw_link_state_toggler
)

set_target_properties(bcm_mpls_utils PROPERTIES COMPILE_FLAGS
  "-DINCLUDE_L3 -DBCM_ESW_SUPPORT"
)

add_library(bcm_cinter
  fboss/agent/hw/bcm/BcmCinter.cpp
)

target_link_libraries(bcm_cinter
  async_logger
  fboss_error
  Folly::folly
  ${OPENNSA}
)

set_target_properties(bcm_cinter PROPERTIES COMPILE_FLAGS
  "-DINCLUDE_L3 -DBCM_ESW_SUPPORT"
)

add_library(sdk_wrap_settings
  fboss/agent/hw/bcm/SdkWrapSettings.cpp
)

target_link_libraries(sdk_wrap_settings
  Folly::folly
)
