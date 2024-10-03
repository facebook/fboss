# CMake to build libraries and binaries in fboss/agent/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(acl_test_utils
  fboss/agent/test/utils/AclTestUtils.cpp
)

target_link_libraries(acl_test_utils
  fboss_error
  hw_switch
  switch_config_cpp2
  switch_state_cpp2
  asic_test_utils
)

add_library(copp_test_utils
  fboss/agent/test/utils/CoppTestUtils.cpp
)

target_link_libraries(copp_test_utils
  asic_test_utils
  Folly::folly
  hw_switch
  load_balancer_test_utils
  packet
  pkt_test_utils
  packet_factory
  resourcelibutil
  switch_asics
  switch_config_cpp2
  test_ensemble_if
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(pkt_test_utils
  fboss/agent/test/utils/PacketSendUtils.cpp
  fboss/agent/test/utils/PacketTestUtils.cpp
)

target_link_libraries(pkt_test_utils
  core
  config_factory
  fboss_types
  network_address_cpp2
)

add_library(fabric_test_utils
  fboss/agent/test/utils/FabricTestUtils.cpp
)

target_link_libraries(fabric_test_utils
  fboss_types
  stats
  switch_config_cpp2
  config_factory
  test_ensemble_if
  common_utils
  ${GTEST}
)

add_library(traffic_policy_utils
  fboss/agent/test/utils/TrafficPolicyTestUtils.cpp
)

target_link_libraries(traffic_policy_utils
  switch_config_cpp2
  config_factory
  switch_asics
  state
  Folly::folly
)

add_library(olympic_qos_utils
  fboss/agent/test/utils/OlympicTestUtils.cpp
)

target_link_libraries(olympic_qos_utils
  fboss_types
  asic_test_utils
  packet_factory
  Folly::folly
  switch_config_cpp2
  traffic_policy_utils
)

add_library(port_test_utils
  fboss/agent/test/utils/PortTestUtils.cpp
)

target_link_libraries(port_test_utils
  fboss_types
  Folly::folly
  switch_config_cpp2
  transceiver_cpp2
  test_ensemble_if
  state
)

add_library(config_utils
  fboss/agent/test/utils/ConfigUtils.cpp
)

target_link_libraries(config_utils
  agent_features
  asic_test_utils
  fboss_types
  Folly::folly
  platform_mapping
  switch_config_cpp2
  test_ensemble_if
  port_test_utils
)

add_library(asic_test_utils
  fboss/agent/test/utils/AsicUtils.cpp
)

target_link_libraries(asic_test_utils
  core
  switch_asics
)


add_library(qos_test_utils
  fboss/agent/test/utils/QosTestUtils.cpp
)

target_link_libraries(qos_test_utils
  asic_test_utils
  ecmp_helper
  fboss_types
  switch_asics
  state
  test_ensemble_if
  Folly::folly
)

add_library(l2learn_observer_util
  fboss/agent/test/utils/L2LearningUpdateObserverUtil.cpp
)

target_link_libraries(l2learn_observer_util
  core
  l2learn_event_observer
  Folly::folly
)

add_library(queue_per_host_test_utils
  fboss/agent/test/utils/QueuePerHostTestUtils.cpp
)


target_link_libraries(queue_per_host_test_utils
  asic_test_utils
  agent_ensemble
  acl_test_utils
  common_utils
  config_utils
  config_factory
  copp_test_utils
  traffic_policy_utils
  load_balancer_test_utils
  hw_switch
  packet
  packet_factory
  ecmp_helper
  fboss_types
  switch_asics
  state
  Folly::folly
)
add_library(load_balancer_test_utils
  fboss/agent/test/utils/LoadBalancerTestUtils.cpp
)

target_link_libraries(load_balancer_test_utils
  ${GTEST}
  state
  Folly::folly
  test_ensemble_if
  core
  utils
  switch_asics
  loadbalancer_utils
  acl_test_utils
  config_utils
  packet
  packet_factory
  resourcelibutil
  common_utils
)

add_library(dscp_marking_utils
  fboss/agent/test/utils/DscpMarkingUtils.cpp
)

target_link_libraries(dscp_marking_utils
  acl_test_utils
  config_utils
  olympic_qos_utils
  traffic_policy_utils
  fboss_types
  packet_factory
  Folly::folly
  switch_config_cpp2
)

add_library(trap_packet_utils
  fboss/agent/test/utils/TrapPacketUtils.cpp
)

target_link_libraries(trap_packet_utils
  fboss_types
  Folly::folly
  platform_config_cpp2
  switch_config_cpp2
  switch_state_cpp2
)

add_library(stats_test_utils
  fboss/agent/test/utils/StatsTestUtils.cpp
)

add_library(
  ecmp_dataplane_test_util
  fboss/agent/test/utils/EcmpDataPlaneTestUtil.cpp
)

target_link_libraries(ecmp_dataplane_test_util
  route_update_wrapper
  packet
  state
  ecmp_helper
  linkstate_toggler
  test_ensemble_if
  load_balancer_test_utils
  fboss_types
  route_update_wrapper
)

add_library(port_stats_test_utils
  fboss/agent/test/utils/PortStatsTestUtils.cpp
)

target_link_libraries(port_stats_test_utils
  fboss_types
  stats
  Folly::folly
  switch_config_cpp2
  FBThrift::thriftcpp2
)

add_library(packet_snooper
  fboss/agent/test/utils/PacketSnooper.cpp
)

target_link_libraries(packet_snooper
  core
  fboss_types
  packet
  packet_factory
  Folly::folly
)

add_library(mac_test_utils
  fboss/agent/test/utils/MacTestUtils.cpp
)

target_link_libraries(mac_test_utils
  state
  test_ensemble_if
  network_address_cpp2
)

add_library(
  load_balancer_test_runner_h
  fboss/agent/test/utils/LoadBalancerTestRunner.h
)


target_link_libraries(load_balancer_test_runner_h
  config_utils
  ecmp_dataplane_test_util
  load_balancer_test_utils
  ${GTEST}

  ecmp_helper
  linkstate_toggler
  test_ensemble_if
  load_balancer_test_utils
  fboss_types
  route_update_wrapper
)

add_library(aqm_test_utils
  fboss/agent/test/utils/AqmTestUtils.cpp
)

target_link_libraries(aqm_test_utils
  switch_asics
  switch_config_cpp2
  fboss_error
  port_test_utils
  test_ensemble_if
  common_utils
  ${GTEST}
)

add_library(agent_hw_test_constants
  fboss/agent/test/utils/AgentHwTestConstants.cpp
)

target_link_libraries(agent_hw_test_constants
  mpls_cpp2
)

add_library(scale_test_utils
  fboss/agent/test/utils/ScaleTestUtils.cpp
)

target_link_libraries(scale_test_utils
  asic_test_utils
  core
  switch_asics
)

add_library(invariant_test_utils
  fboss/agent/test/utils/InvariantTestUtils.cpp
)

target_link_libraries(invariant_test_utils
  config_utils
  copp_test_utils
  load_balancer_test_utils
  qos_test_utils
  packet
  packet_factory
  test_ensemble_if
  validated_shell_commands_cpp2
)

add_library(route_test_utils
  fboss/agent/test/utils/RouteTestUtils.cpp
)

target_link_libraries(route_test_utils
  route_update_wrapper
  ctrl_cpp2
  route_distribution_gen
)

add_library(queue_test_utils
  fboss/agent/test/utils/QueueTestUtils.cpp
)

target_link_libraries(queue_test_utils
  config_utils
  olympic_qos_utils
  switch_asics
  switch_config_cpp2
)

add_library(mirror_test_utils
  fboss/agent/test/utils/MirrorTestUtils.cpp
)

target_link_libraries(mirror_test_utils
  config_utils
  fboss_types
  trap_packet_utils
  switch_config_cpp2
  Folly::folly
)

add_library(dsf_config_utils
  fboss/agent/test/utils/DsfConfigUtils.cpp
)

target_link_libraries(dsf_config_utils
  asic_test_utils
  config_utils
  switch_config_cpp2
  switch_asics
)

add_library(voq_test_utils
  fboss/agent/test/utils/VoqTestUtils.cpp
)

target_link_libraries(voq_test_utils
  dsf_config_utils
  config_factory
  fboss_types
  switchid_scope_resolver
  switch_config_cpp2
  state
  ecmp_helper
  test_ensemble_if
)
