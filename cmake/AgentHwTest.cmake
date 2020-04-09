# CMake to build libraries and binaries in fboss/agent/hw/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(config_factory
  fboss/agent/hw/test/ConfigFactory.cpp
)

target_link_libraries(config_factory
  fboss_types
  hw_switch
  switch_config_cpp2
  Folly::folly
)

add_library(hw_test_main
  fboss/agent/hw/test/Main.cpp
)

target_link_libraries(hw_test_main
  fboss_init
  Folly::folly
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

add_library(hw_switch_ensemble
  fboss/agent/hw/test/HwSwitchEnsemble.cpp
)

target_link_libraries(hw_switch_ensemble
  hw_link_state_toggler
  core
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
  packet_factory
  state
  Folly::folly
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

add_library(hw_switch_test
  fboss/agent/hw/test/HwLinkStateDependentTest.cpp
  fboss/agent/hw/test/HwTest.cpp
  fboss/agent/hw/test/HwTestConstants.cpp
  fboss/agent/hw/test/HwTestStatUtils.cpp
  fboss/agent/hw/test/HwTestCoppUtils.cpp
  fboss/agent/hw/test/HwRouteScaleTest.cpp
  fboss/agent/hw/test/HwRouteOverflowTest.cpp
  fboss/agent/hw/test/HwVlanTests.cpp
  fboss/agent/hw/test/HwL2ClassIDTests.cpp
  fboss/agent/hw/test/HwPortStressTests.cpp

  fboss/agent/hw/test/dataplane_tests/HwCoppTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.cpp
  fboss/agent/hw/test/dataplane_tests/HwJumboFramesTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwInDiscardCounterTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwInPauseDiscardsTests.cpp
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
  fboss/agent/hw/test/dataplane_tests/HwSendPacketToQueueTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwTrunkLoadBalancerTests.cpp
  fboss/agent/hw/test/dataplane_tests/HwWatermarkTests.cpp
)

target_link_libraries(hw_switch_test
  config_factory
  hw_packet_utils
  hw_switch_ensemble
  load_balancer_utils
  traffic_policy_utils
  core
  hardware_stats_cpp2
  route_distribution_gen
  route_scale_gen
  trunk_utils
  Folly::folly
)
