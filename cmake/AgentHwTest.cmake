# CMake to build libraries and binaries in fboss/agent/hw/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(config_factory
  fboss/agent/hw/test/ConfigFactory.cpp
  fboss/agent/hw/test/HwPortUtils.cpp
)

target_link_libraries(config_factory
  config_utils
  fboss_types
  hw_switch
  hwagent
  switch_config_cpp2
  Folly::folly
  fboss_config_utils
  port_test_utils
  linkstate_toggler
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

add_library(hw_packet_utils
  fboss/agent/hw/test/HwTestLearningUpdateObserver.cpp
  fboss/agent/hw/test/HwTestLinkScanUpdateObserver.cpp
  fboss/agent/hw/test/HwTestPacketSnooper.cpp
  fboss/agent/hw/test/HwTestPacketUtils.cpp
)

target_link_libraries(hw_packet_utils
  hw_switch_ensemble
  packet_factory
  packet_snooper
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

add_library(l2learn_event_observer
  fboss/agent/L2LearnEventObserver.cpp
)

target_link_libraries(l2learn_event_observer
  fboss_error
  Folly::folly
)

add_library(hw_copp_utils
  fboss/agent/hw/test/HwTestCoppUtils.cpp
)

target_link_libraries(hw_copp_utils
  hw_test_acl_utils
  switch_asics
  packet_factory
  Folly::folly
  resourcelibutil
  switch_config_cpp2
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(hw_qos_utils
  fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.cpp
)

target_link_libraries(hw_qos_utils
  hw_packet_utils
  packet_factory
  Folly::folly
  qos_test_utils
  resourcelibutil
  switch_config_cpp2
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(hw_acl_utils
  fboss/agent/hw/test/HwTestAclUtils.cpp
)

target_link_libraries(hw_acl_utils
  hw_packet_utils
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

add_library(hw_link_state_toggler
  fboss/agent/test/LinkStateToggler.cpp
)

target_link_libraries(hw_link_state_toggler
  core
  switch_config_cpp2
  state
  Folly::folly
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
  pkt_test_utils
  test_ensemble_if
  sw_switch_warmboot_helper
  multiswitch_test_server
  split_agent_thrift_syncer
)

add_library(hw_test_acl_utils
    fboss/agent/hw/test/HwTestAclUtils.cpp
)

target_link_libraries(hw_test_acl_utils
  acl_test_utils
  hw_switch
  switch_asics
  state
)

add_library(load_balancer_utils
  fboss/agent/hw/test/LoadBalancerUtils.cpp
)

target_link_libraries(load_balancer_utils
  fboss_types
  hw_switch
  platform_base
  switch_asics
  state
  loadbalancer_utils
)

add_library(prod_config_utils
  fboss/agent/hw/test/HwTestProdConfigUtils.cpp
)

target_link_libraries(prod_config_utils
  copp_test_utils
  load_balancer_utils
  switch_config_cpp2
  traffic_policy_utils
  olympic_qos_utils
  hw_copp_utils
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
  fboss/agent/hw/test/HwFlexPortTests.cpp
  fboss/agent/hw/test/HwIngressBufferTests.cpp
  fboss/agent/hw/test/HwEcmpTrunkTests.cpp
  fboss/agent/hw/test/HwLabelEdgeRouteTest.cpp
  fboss/agent/hw/test/HwLabelSwitchRouteTest.cpp
  fboss/agent/hw/test/HwLinkStateDependentTest.cpp
  fboss/agent/hw/test/HwMirrorTests.cpp
  fboss/agent/hw/test/HwTest.cpp
  fboss/agent/hw/test/HwTestAclUtils.cpp
  fboss/agent/hw/test/HwTestPortUtils.cpp
  fboss/agent/hw/test/HwTestCoppUtils.cpp
  fboss/agent/hw/test/HwRouteTests.cpp
  fboss/agent/hw/test/HwTrunkTests.cpp
  fboss/agent/hw/test/HwVlanTests.cpp
  fboss/agent/hw/test/HwVerifyPfcConfigInHwTest.cpp
  fboss/agent/hw/test/HwAclStatTests.cpp
  fboss/agent/hw/test/HwPortTests.cpp
  fboss/agent/hw/test/HwPortLedTests.cpp
  fboss/agent/hw/test/HwPortProfileTests.cpp
  fboss/agent/hw/test/HwPortStressTests.cpp
  fboss/agent/hw/test/HwResourceStatsTests.cpp
  fboss/agent/hw/test/HwSwitchStateReplayTest.cpp
  fboss/agent/hw/test/HwParityErrorTest.cpp
  fboss/agent/hw/test/HwPtpTcTests.cpp
  fboss/agent/hw/test/HwTeFlowTestUtils.cpp
  fboss/agent/hw/test/HwTeFlowTests.cpp
  fboss/agent/hw/test/HwUdfTests.cpp
  fboss/agent/hw/test/HwTestPfcUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwAqmTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwInPauseDiscardsTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwIpInIpTunnelTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwMPLSTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwTeFlowTrafficTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwTrafficPfcTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwProdInvariantHelper.cpp
  fboss/agent/hw/test/dataplane_tests/HwProdInvariantTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwPacketSendTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwSflowTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.cpp
  fboss/agent/hw/test/dataplane_tests/HwTrunkLoadBalancerTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwRouteStatTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTestsV6Roce.cpp
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
  acl_test_utils
  aqm_test_utils
  config_utils
  aqm_test_utils
  copp_test_utils
  dscp_marking_utils
  ecmp_dataplane_test_util
  hw_packet_utils
  hw_switch_ensemble
  voq_test_utils
  linkstate_toggler
  load_balancer_utils
  mac_test_utils
  mirror_test_utils
  olympic_qos_utils
  prod_config_factory
  prod_config_utils
  qos_test_utils
  test_ensemble_if
  traffic_policy_utils
  stats_test_utils
  core
  sflow_shim_utils
  hardware_stats_cpp2
  queue_per_host_test_utils
  route_distribution_gen
  route_scale_gen
  trunk_utils
  trap_packet_utils
  Folly::folly
  validated_shell_commands_cpp2
  hwswitch_matcher
  switchid_scope_resolver
  hw_stat_printers
  port_stats_test_utils
  agent_hw_test_constants
  sai_switch_ensemble
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
  dscp_marking_utils
  olympic_qos_utils
  queue_per_host_test_utils
  load_balancer_utils
  load_balancer_test_utils
  hw_pfc_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(multiswitch_test_server
  fboss/agent/test/MultiSwitchTestServer.cpp
)

target_link_libraries(multiswitch_test_server
  core
  handler
  multiswitch_service
  Folly::folly
)

add_library(hw_test_fabric_utils
  fboss/agent/hw/test/HwTestFabricUtils.cpp
)

target_link_libraries(hw_test_fabric_utils
  hw_switch
  config_factory
  fboss_types
  switch_config_cpp2
  ${GTEST}
)
