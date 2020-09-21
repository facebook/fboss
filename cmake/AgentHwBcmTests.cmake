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
  fboss/agent/hw/bcm/tests/HwTestAclUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestEcmpUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestNeighborUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestMacUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestMplsUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestPacketTrapEntry.cpp
  fboss/agent/hw/bcm/tests/HwVlanUtils.cpp
  fboss/agent/hw/bcm/tests/BcmAclCoppTests.cpp
  fboss/agent/hw/bcm/tests/BcmAclUnitTests.cpp
  fboss/agent/hw/bcm/tests/BcmAddDelEcmpTests.cpp
  fboss/agent/hw/bcm/tests/BcmBstStatsMgrTest.cpp
  fboss/agent/hw/bcm/tests/BcmControlPlaneTests.cpp
  fboss/agent/hw/bcm/tests/BcmCosQueueManagerTest.cpp
  fboss/agent/hw/bcm/tests/BcmCosQueueManagerCounterTests.cpp
  fboss/agent/hw/bcm/tests/BcmDeathTests.cpp
  fboss/agent/hw/bcm/tests/BcmEcmpTests.cpp
  fboss/agent/hw/bcm/tests/BcmEcmpTrunkTests.cpp
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
  fboss/agent/hw/bcm/tests/BcmMirrorTests.cpp
  fboss/agent/hw/bcm/tests/BcmMplsTestUtils.cpp
  fboss/agent/hw/bcm/tests/BcmPortQueueManagerTests.cpp
  fboss/agent/hw/bcm/tests/BcmPortTests.cpp
  fboss/agent/hw/bcm/tests/BcmPortStressTests.cpp
  fboss/agent/hw/bcm/tests/BcmQosPolicyTests.cpp
  fboss/agent/hw/bcm/tests/BcmQosMapTests.cpp
  fboss/agent/hw/bcm/tests/BcmQueueStatCollectionTests.cpp
  fboss/agent/hw/bcm/tests/BcmRtag7Test.cpp
  fboss/agent/hw/bcm/tests/BcmRouteTests.cpp
  fboss/agent/hw/bcm/tests/BcmStateDeltaTests.cpp
  fboss/agent/hw/bcm/tests/BcmSwitchStateReplayTest.cpp
  fboss/agent/hw/bcm/tests/BcmTestRouteUtils.cpp
  fboss/agent/hw/bcm/tests/BcmTestStatUtils.cpp
  fboss/agent/hw/bcm/tests/BcmTrunkTests.cpp
  fboss/agent/hw/bcm/tests/BcmTrunkUtils.cpp
  fboss/agent/hw/bcm/tests/BcmUnitTests.cpp
  fboss/agent/hw/bcm/tests/QsetCmpTests.cpp
  fboss/agent/hw/bcm/tests/HwTestRouteUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestQosUtils.cpp
  fboss/agent/hw/bcm/tests/HwTestCoppUtils.cpp
  fboss/agent/hw/bcm/tests/oss/BcmSwitchEnsemble.cpp

  fboss/agent/hw/bcm/tests/dataplane_tests/BcmSflowTests.cpp
  fboss/agent/hw/bcm/tests/dataplane_tests/BcmMultiAqmProfileTests.cpp
  fboss/agent/hw/bcm/tests/dataplane_tests/BcmConfigVerifyQosTests.cpp
  fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.cpp
  fboss/agent/hw/bcm/tests/dataplane_tests/BcmMirroringTests.cpp
  fboss/agent/hw/bcm/tests/dataplane_tests/BcmQcmDataTests.cpp
  fboss/agent/hw/bcm/tests/dataplane_tests/BcmConfigSetupTests.cpp
)

target_compile_definitions(bcm_test
  PUBLIC
    ${LIBGMOCK_DEFINES}
)

target_include_directories(bcm_test
  PUBLIC
    ${LIBGMOCK_INCLUDE_DIR}
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
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
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

add_library(bcm_stat_utils
  fboss/agent/hw/bcm/tests/BcmTestStatUtils.cpp
)

target_link_libraries(bcm_stat_utils
  bcm
  bcm_qos_utils
)

add_library(bcm_copp_utils
  fboss/agent/hw/bcm/tests/HwTestCoppUtils.cpp
)

target_link_libraries(bcm_copp_utils
  bcm
  bcm_stat_utils
  hw_copp_utils
)

add_library(bcm_packet_trap_helper
  fboss/agent/hw/bcm/tests/HwTestPacketTrapEntry.cpp
)

target_link_libraries(bcm_packet_trap_helper
  bcm
)
