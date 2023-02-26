# CMake to build libraries and binaries in fboss/agent/hw/bcm/tests

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(bcm_test
  fboss/agent/hw/bcm/tests/BcmTest.cpp
  fboss/agent/hw/bcm/tests/BcmTestUtils.cpp
  fboss/agent/hw/bcm/tests/BcmPortUtils.cpp
  fboss/agent/hw/bcm/tests/BcmColdBootStateTests.cpp
  fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.cpp
  fboss/agent/hw/bcm/tests/HwSwitchEnsembleFactory.cpp
  fboss/agent/hw/bcm/tests/HwTestTamUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestAclUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestPfcUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestAqmUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestEcmpUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestFabricUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestPtpTcUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestNeighborUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestMirrorUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestMplsUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestTrunkUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestPacketTrapEntry.cpp
  fboss/agent/hw/bcm/tests/HwTestPortUtils.cpp
  fboss/agent/hw/bcm/tests/HwVlanUtils.cpp
  fboss/agent/hw/bcm/tests/BcmAclCoppTests.cpp
  fboss/agent/hw/bcm/tests/BcmAclNexthopTests.cpp
  fboss/agent/hw/bcm/tests/BcmAclUnitTests.cpp
  fboss/agent/hw/bcm/tests/BcmAddDelEcmpTests.cpp
  fboss/agent/hw/bcm/tests/BcmBstStatsMgrTest.cpp
  fboss/agent/hw/bcm/tests/BcmControlPlaneTests.cpp
  fboss/agent/hw/bcm/tests/BcmCosQueueManagerTest.cpp
  fboss/agent/hw/bcm/tests/BcmCosQueueManagerCounterTests.cpp
  fboss/agent/hw/bcm/tests/BcmDeathTests.cpp
  fboss/agent/hw/bcm/tests/BcmEcmpTests.cpp
  fboss/agent/hw/bcm/tests/BcmEmptyEcmpTests.cpp
  fboss/agent/hw/bcm/tests/BcmFieldProcessorTests.cpp
  fboss/agent/hw/bcm/tests/BcmHostKeyTests.cpp
  fboss/agent/hw/bcm/tests/BcmHostTests.cpp
  fboss/agent/hw/bcm/tests/BcmInterfaceTests.cpp
  fboss/agent/hw/bcm/tests/BcmLabeledEgressTest.cpp
  fboss/agent/hw/bcm/tests/BcmLabelSwitchActionTests.cpp
  fboss/agent/hw/bcm/tests/BcmLabelForwardingTests.cpp
  fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.cpp
  fboss/agent/hw/bcm/tests/BcmLinkStateToggler.cpp
  fboss/agent/hw/bcm/tests/BcmMplsTestUtils.cpp
  fboss/agent/hw/bcm/tests/BcmPortQueueManagerTests.cpp
  fboss/agent/hw/bcm/tests/BcmPortIngressBufferManagerTests.cpp
  fboss/agent/hw/bcm/tests/BcmPortTests.cpp
  fboss/agent/hw/bcm/tests/BcmPortStressTests.cpp
  fboss/agent/hw/bcm/tests/BcmQosPolicyTests.cpp
  fboss/agent/hw/bcm/tests/BcmQosMapTests.cpp
  fboss/agent/hw/bcm/tests/BcmQueueStatCollectionTests.cpp
  fboss/agent/hw/bcm/tests/BcmRtag7Test.cpp
  fboss/agent/hw/bcm/tests/BcmRouteTests.cpp
  fboss/agent/hw/bcm/tests/BcmStateDeltaTests.cpp
  fboss/agent/hw/bcm/tests/HwTestTeFlowUtils.cpp
  fboss/agent/hw/bcm/tests/BcmTrunkTests.cpp
  fboss/agent/hw/bcm/tests/BcmTrunkUtils.cpp
  fboss/agent/hw/bcm/tests/BcmUnitTests.cpp
  fboss/agent/hw/bcm/tests/QsetCmpTests.cpp
  fboss/agent/hw/bcm/tests/HwTestRouteUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestQosUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestCoppUtils.cpp
  fboss/agent/hw/bcm/tests/oss/BcmSwitchEnsemble.cpp

  fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.cpp
  fboss/agent/hw/bcm/tests/dataplane_tests/BcmQcmDataTests.cpp
)

target_compile_definitions(bcm_test
  PUBLIC
    ${LIBGMOCK_DEFINES}
)

target_include_directories(bcm_test
  PUBLIC
    ${LIBGMOCK_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(bcm_test
  handler
  setup_thrift
  bcm
  bcm_mpls_utils
  config
  counter_utils
  # --whole-archive is needed for gtest to find these tests
  -Wl,--whole-archive
  hw_switch_test
  hw_test_main
  -Wl,--no-whole-archive
  product_info
  bcm_test_platforms
  ecmp_helper
  route_scale_gen
  trunk_utils
  sflow_cpp2
  packettrace_cpp2
  wedge_led_utils
  bcm_phy_capabilities
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(bcm_phy_capabilities
  fboss/agent/hw/bcm/tests/PhyCapabilities.cpp
)

target_link_libraries(bcm_phy_capabilities
  bcm
)

add_library(bcm_switch_ensemble
  fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.cpp
  fboss/agent/hw/bcm/tests/HwSwitchEnsembleFactory.cpp
  fboss/agent/hw/bcm/tests/oss/BcmSwitchEnsemble.cpp
)

target_link_libraries(bcm_switch_ensemble
  config
  core
  bcm
  bcm_link_state_toggler
  hw_stat_utils
  hw_switch_ensemble
  bcm_test_platforms
  wedge_led_utils
  Folly::folly
)

add_library(hw_stat_utils
  fboss/agent/hw/test/HwTestStatUtils.cpp
)

target_link_libraries(hw_stat_utils
  hw_switch
  stats
  hardware_stats_cpp2
  Folly::folly
)

add_library(bcm_ecmp_utils
  fboss/agent/hw/bcm/tests/HwTestEcmpUtils.cpp
)

target_link_libraries(bcm_ecmp_utils
  bcm
)

add_library(bcm_ptp_tc_utils
  fboss/agent/hw/bcm/tests/HwTestPtpTcUtils.cpp
)

target_link_libraries(bcm_ptp_tc_utils
  bcm
)

add_library(bcm_port_utils
  fboss/agent/hw/bcm/tests/HwTestPortUtils.cpp
)

target_link_libraries(bcm_port_utils
  bcm
)

add_library(bcm_qos_utils
  fboss/agent/hw/bcm/tests/HwTestQosUtils.cpp
  fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.cpp
)

target_link_libraries(bcm_qos_utils
  bcm
)

add_library(bcm_trunk_utils
  fboss/agent/hw/bcm/tests/HwTestTrunkUtils.cpp
  fboss/agent/hw/bcm/tests/BcmTrunkUtils.cpp
)

target_link_libraries(bcm_trunk_utils
  bcm
)

add_library(bcm_copp_utils
  fboss/agent/hw/bcm/tests/HwTestCoppUtils.cpp
)

target_link_libraries(bcm_copp_utils
  bcm
  hw_copp_utils
  traffic_policy_utils
)

add_library(bcm_teflow_utils
	fboss/agent/hw/bcm/tests/HwTestTeFlowUtils.cpp
)

target_link_libraries(bcm_teflow_utils
  bcm
)

add_library(bcm_packet_trap_helper
  fboss/agent/hw/bcm/tests/HwTestPacketTrapEntry.cpp
)

target_link_libraries(bcm_packet_trap_helper
  bcm
)

add_executable(bcm_multinode_test
  fboss/agent/hw/bcm/tests/BcmMultiNodeTest.cpp
)

target_link_libraries(bcm_multinode_test
  multinode_tests
  platform
  bcm
  bcm_ecmp_utils
  bcm_qos_utils
  bcm_trunk_utils
)
