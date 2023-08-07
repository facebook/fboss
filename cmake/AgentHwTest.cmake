# CMake to build libraries and binaries in fboss/agent/hw/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(config_factory
  fboss/agent/hw/test/ConfigFactory.cpp
  fboss/agent/hw/test/HwPortUtils.cpp
)

target_link_libraries(config_factory
  fboss_types
  hw_switch
  switch_config_cpp2
  Folly::folly
  fboss_config_utils
)

add_library(hw_test_main
  fboss/agent/hw/test/Main.cpp
)

target_link_libraries(hw_test_main
  fboss_init
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(hw_agent_packet_utils
  fboss/agent/hw/test/HwAgentTestPacketSnooper.cpp
)

target_link_libraries(hw_agent_packet_utils
  Folly::folly
  packet_factory
)

add_library(hw_packet_utils
  fboss/agent/hw/test/HwTestLearningUpdateObserver.cpp
  fboss/agent/hw/test/HwTestPacketSnooper.cpp
  fboss/agent/hw/test/HwTestPacketUtils.cpp
)

target_link_libraries(hw_packet_utils
  hw_switch_ensemble
  packet_factory
  Folly::folly
  resourcelibutil
)

add_library(packet_observer
  fboss/agent/PacketObserver.cpp
)

target_link_libraries(packet_observer
  fboss_error
  Folly::folly
)

add_library(hw_copp_utils
  fboss/agent/hw/test/HwTestCoppUtils.cpp
)

target_link_libraries(hw_copp_utils
  switch_asics
  packet_factory
  Folly::folly
  resourcelibutil
  switch_config_cpp2
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(hw_teflow_utils
  fboss/agent/hw/test/HwTeFlowTestUtils.cpp
)

target_link_libraries(hw_teflow_utils
  config_factory
  hw_switch_ensemble
  state
  Folly::folly
  switch_config_cpp2
)

add_library(hw_olympic_qos_utils
  fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.cpp
)

target_link_libraries(hw_olympic_qos_utils
  fboss_types
  hw_switch_ensemble
  packet_factory
  Folly::folly
  switch_config_cpp2
)

add_library(hw_dscp_marking_utils
  fboss/agent/hw/test/dataplane_tests/HwDscpMarkingTests.cpp
)

target_link_libraries(hw_dscp_marking_utils
  fboss_types
  hw_switch_ensemble
  packet_factory
  Folly::folly
  switch_config_cpp2
)

add_library(hw_link_state_toggler
  fboss/agent/hw/test/HwLinkStateToggler.cpp
)

target_link_libraries(hw_link_state_toggler
  core
  switch_config_cpp2
  state
  Folly::folly
)

add_library(hw_test_utils
  fboss/agent/hw/test/dataplane_tests/HwTestUtils.cpp
)

target_link_libraries(hw_test_utils
  fboss_types
  hardware_stats_cpp2
  agent_test_utils
)

add_library(hw_switch_ensemble
  fboss/agent/hw/test/HwSwitchEnsemble.cpp
  fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.cpp
  fboss/agent/hw/test/StaticL2ForNeighborHwSwitchUpdater.cpp
)

target_link_libraries(hw_switch_ensemble
  hw_link_state_toggler
  switchid_scope_resolver
  core
  hw_test_utils
  test_ensemble_if
)

add_library(load_balancer_utils
  fboss/agent/hw/test/LoadBalancerUtils.cpp
)

target_link_libraries(load_balancer_utils
  hw_packet_utils
  core
  fboss_types
  switch_config_cpp2
  hw_switch_ensemble
  resourcelibutil
  packet_factory
  state
  Folly::folly
)

add_library(prod_config_utils
  fboss/agent/hw/test/HwTestProdConfigUtils.cpp
)

target_link_libraries(prod_config_utils
  load_balancer_utils
  switch_config_cpp2
  hw_olympic_qos_utils
  hw_copp_utils
  hw_switch_test
)

add_library(traffic_policy_utils
  fboss/agent/hw/test/TrafficPolicyUtils.cpp
)

target_link_libraries(traffic_policy_utils
  switch_config_cpp2
  config_factory
  state
  Folly::folly
)

add_fbthrift_cpp_library(
  validated_shell_commands_cpp2
  fboss/agent/validated_shell_commands.thrift
  OPTIONS
    json
    visitation
)

set(hw_switch_test_srcs
  fboss/agent/hw/test/HwEcmpTests.cpp
  fboss/agent/hw/test/HwTestFabricUtils.cpp
  fboss/agent/hw/test/HwFabricSwitchTests.cpp
  fboss/agent/hw/test/HwFlexPortTests.cpp
  fboss/agent/hw/test/HwEcmpTrunkTests.cpp
  fboss/agent/hw/test/HwLabelEdgeRouteTest.cpp
  fboss/agent/hw/test/HwLabelSwitchRouteTest.cpp
  fboss/agent/hw/test/HwLinkStateDependentTest.cpp
  fboss/agent/hw/test/HwMirrorTests.cpp
  fboss/agent/hw/test/HwNeighborTests.cpp
  fboss/agent/hw/test/HwTest.cpp
  fboss/agent/hw/test/HwTestAclUtils.cpp
  fboss/agent/hw/test/HwTestConstants.cpp
  fboss/agent/hw/test/HwTestMacUtils.cpp
  fboss/agent/hw/test/HwTestPortUtils.cpp
  fboss/agent/hw/test/HwTestStatUtils.cpp
  fboss/agent/hw/test/HwTestCoppUtils.cpp
  fboss/agent/hw/test/HwRouteScaleTest.cpp
  fboss/agent/hw/test/HwRouteTests.cpp
  fboss/agent/hw/test/HwTrunkTests.cpp
  fboss/agent/hw/test/HwVlanTests.cpp
  fboss/agent/hw/test/HwVoqSwitchTests.cpp
  fboss/agent/hw/test/HwL2ClassIDTests.cpp
  fboss/agent/hw/test/HwAclMatchActionsTests.cpp
  fboss/agent/hw/test/HwAclPriorityTests.cpp
  fboss/agent/hw/test/HwAclQualifierTests.cpp
  fboss/agent/hw/test/HwPfcTests.cpp
  fboss/agent/hw/test/HwAclStatTests.cpp
  fboss/agent/hw/test/HwPortTests.cpp
  fboss/agent/hw/test/HwDiagShellStressTest.cpp
  fboss/agent/hw/test/HwPortLedTests.cpp
  fboss/agent/hw/test/HwPortProfileTests.cpp
  fboss/agent/hw/test/HwPortStressTests.cpp
  fboss/agent/hw/test/HwResourceStatsTests.cpp
  fboss/agent/hw/test/HwRxReasonTests.cpp
  fboss/agent/hw/test/HwSwitchStateReplayTest.cpp
  fboss/agent/hw/test/HwParityErrorTest.cpp
  fboss/agent/hw/test/HwPtpTcTests.cpp
  fboss/agent/hw/test/HwTeFlowTestUtils.cpp
  fboss/agent/hw/test/HwTeFlowTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwAclCounterTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwConfigSetupTest.cpp
  fboss/agent/hw/test/dataplane_tests/HwConfigVerifyQosTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwCoppTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwDscpMarkingTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwDscpQueueMappingTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.cpp
  fboss/agent/hw/test/dataplane_tests/HwAqmTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwJumboFramesTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwInDiscardCounterTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwInPauseDiscardsTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwIpInIpTunnelTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwL4PortBlackholingTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwMPLSTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwMacLearningTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV4.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV6.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV4ToMpls.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV6ToMpls.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV4InMplsPhp.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV6InMplsPhp.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV4InMplsSwap.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV6InMplsSwap.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoopBackTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwRouteOverDifferentAddressFamilyNhopTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwMacLearningAndNeighborResolutionTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwMirroringTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwMmuTuningTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwOlympicQosTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwOlympicQosSchedulerTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwOverflowTest.cpp
  fboss/agent/hw/test/dataplane_tests/HwTeFlowTrafficTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwTrafficPfcTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwProdInvariantHelper.cpp
  fboss/agent/hw/test/dataplane_tests/HwProdInvariantTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwPacketSendTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwPortBandwidthTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwQueuePerHostL2Tests.cpp
  fboss/agent/hw/test/dataplane_tests/HwQueuePerHostTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwQueuePerHostRouteTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwRouteOverflowTest.cpp
  fboss/agent/hw/test/dataplane_tests/HwSendPacketToQueueTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwSflowTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwSflowMirrorTest.cpp
  fboss/agent/hw/test/dataplane_tests/HwSwitchStatsTxCounterTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwTest2QueueUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestDscpMarkingUtils.cpp
  fboss/agent/hw/test/dataplane_tests/Hw2QueueToOlympicQoSTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTrunkLoadBalancerTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwWatermarkTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwRouteStatTests.cpp
)

if (NOT BUILD_SAI_FAKE)

# Hash polarization packet utilities consume significant amount of
# memory and causes on-diff to fail. Skip including Hash
# polarization files for fake as we do not run these hw tests on fake.

set(hw_switch_test_srcs
  ${hw_switch_test_srcs}
  fboss/agent/hw/test/HwHashPolarizationTestUtils.cpp
  fboss/agent/hw/test/HwTestFullHashedPacketsForSaiTomahawk.cpp
  fboss/agent/hw/test/HwTestFullHashedPacketsForSaiTrident2.cpp
  fboss/agent/hw/test/HwTestFullHashedPacketsForTomahawk.cpp
  fboss/agent/hw/test/HwTestFullHashedPacketsForTomahawk3.cpp
  fboss/agent/hw/test/HwTestFullHashedPacketsForTomahawk4.cpp
  fboss/agent/hw/test/HwTestFullHashedPacketsForTrident2.cpp
  fboss/agent/hw/test/dataplane_tests/HwHashPolarizationTests.cpp
)
endif()

add_library(hw_switch_test
  ${hw_switch_test_srcs}
)

target_link_libraries(hw_switch_test
  config_factory
  agent_test_utils
  hw_packet_utils
  hw_switch_ensemble
  hw_voq_utils
  load_balancer_utils
  prod_config_factory
  prod_config_utils
  traffic_policy_utils
  core
  sflow_shim_utils
  hardware_stats_cpp2
  route_distribution_gen
  route_scale_gen
  trunk_utils
  Folly::folly
  validated_shell_commands_cpp2
  hwswitch_matcher
  switchid_scope_resolver
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(hw_pfc_utils
  fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.cpp
)

target_link_libraries(hw_pfc_utils
  hw_switch_ensemble
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(prod_config_factory
  fboss/agent/hw/test/ProdConfigFactory.cpp
)

target_link_libraries(prod_config_factory
  config_factory
  hw_copp_utils
  hw_dscp_marking_utils
  hw_olympic_qos_utils
  hw_queue_per_host_utils
  load_balancer_utils
  hw_pfc_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(hw_queue_per_host_utils
  fboss/agent/hw/test/HwTestAclUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.cpp
)

target_link_libraries(hw_queue_per_host_utils
  traffic_policy_utils
  fboss_types
  hw_switch
  switch_config_cpp2
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(hw_linkstate_toggler
  fboss/agent/hw/test/HwLinkStateToggler.cpp
)

target_link_libraries(hw_linkstate_toggler
  hw_switch
  state
  core
)

add_library(hw_voq_utils
  fboss/agent/hw/test/HwVoqUtils.cpp
)

target_link_libraries(hw_voq_utils
  config_factory
  fboss_types
  switchid_scope_resolver
  switch_asics
  switch_config_cpp2
  state
  Folly::folly
)
