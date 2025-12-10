# CMake to build libraries and binaries in fboss/agent/hw/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(agent_hw_test_src
  fboss/agent/test/agent_hw_tests/Agent2QueueToOlympicQoSTests.cpp
  fboss/agent/test/agent_hw_tests/AgentCoppTests.cpp
  fboss/agent/test/agent_hw_tests/AgentDot1qMappingTest.cpp
  fboss/agent/test/agent_hw_tests/AgentDscpMarkingTests.cpp
  fboss/agent/test/agent_hw_tests/AgentDscpQueueMappingTests.cpp
  fboss/agent/test/agent_hw_tests/AgentDeepPacketInspectionTests.cpp
  fboss/agent/test/agent_hw_tests/AgentDiagShellStressTests.cpp
  fboss/agent/test/agent_hw_tests/AgentEcmpSpilloverTests.cpp
  fboss/agent/test/agent_hw_tests/AgentEcmpTests.cpp
  fboss/agent/test/agent_hw_tests/AgentEmptyTests.cpp
  fboss/agent/test/agent_hw_tests/AgentEgressForwardingDiscardCounterTests.cpp
  fboss/agent/test/agent_hw_tests/AgentArsBase.cpp
  fboss/agent/test/agent_hw_tests/AgentFlowletSwitchingTests.cpp
  fboss/agent/test/agent_hw_tests/AgentRouteOverDifferentAddressFamilyNhopTests.cpp
  fboss/agent/test/agent_hw_tests/AgentAclInDiscardCounterTests.cpp
  fboss/agent/test/agent_hw_tests/AgentJumboFramesTests.cpp
  fboss/agent/test/agent_hw_tests/AgentIngressBufferTests.cpp
  fboss/agent/test/agent_hw_tests/AgentInNullRouteDiscardsTest.cpp
  fboss/agent/test/agent_hw_tests/AgentInTrapDiscardsTest.cpp
  fboss/agent/test/agent_hw_tests/AgentPacketSendTests.cpp
  fboss/agent/test/agent_hw_tests/AgentL3ForwardingTests.cpp
  fboss/agent/test/agent_hw_tests/AgentL4PortBlackholingTests.cpp
  fboss/agent/test/agent_hw_tests/AgentMacLearningTests.cpp
  fboss/agent/test/agent_hw_tests/AgentMacLearningAndNeighborResolutionTests.cpp
  fboss/agent/test/agent_hw_tests/AgentMirroringTests.cpp
  fboss/agent/test/agent_hw_tests/AgentMirroringScaleTests.cpp
  fboss/agent/test/agent_hw_tests/AgentNSFScaleTests.cpp
  fboss/agent/test/agent_hw_tests/AgentNeighborTests.cpp
  fboss/agent/test/agent_hw_tests/AgentOlympicQosTests.cpp
  fboss/agent/test/agent_hw_tests/AgentOlympicQosSchedulerTests.cpp
  fboss/agent/test/agent_hw_tests/AgentNetworkAIQosTests.cpp
  fboss/agent/test/agent_hw_tests/AgentNetworkAIQosSchedulerTests.cpp
  fboss/agent/test/agent_hw_tests/AgentQueuePerHostL2Tests.cpp
  fboss/agent/test/agent_hw_tests/AgentQueuePerHostTests.cpp
  fboss/agent/test/agent_hw_tests/AgentQueuePerHostRouteTests.cpp
  fboss/agent/test/agent_hw_tests/AgentRouteTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqSwitchFullScaleDsfTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqSwitchLineRateTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqSwitchWithFabricPortsTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqSwitchWithMultipleDsfNodesTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqConditionalEntropyTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqSwitchInterruptsTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqSwitchFirmwareTests.cpp
  fboss/agent/test/agent_hw_tests/AgentFabricSwitchTests.cpp
  fboss/agent/test/agent_hw_tests/AgentPortBandWidthTests.cpp
  fboss/agent/test/agent_hw_tests/AgentPortLedTests.cpp
  fboss/agent/test/agent_hw_tests/AgentPrbsTests.cpp
  fboss/agent/test/agent_hw_tests/AgentAclCounterTests.cpp
  fboss/agent/test/agent_hw_tests/AgentAqmTests.cpp
  fboss/agent/test/agent_hw_tests/AgentOverflowTestBase.cpp
  fboss/agent/test/agent_hw_tests/AgentQosTestBase.cpp
  fboss/agent/test/agent_hw_tests/AgentQosSchedulerTestBase.cpp
  fboss/agent/test/agent_hw_tests/AgentLoopBackTests.cpp
  fboss/agent/test/agent_hw_tests/AgentSendPacketToQueueTests.cpp
  fboss/agent/test/agent_hw_tests/AgentWatermarkTests.cpp
  fboss/agent/test/agent_hw_tests/AgentSwitchStatsTxCounterTests.cpp
  fboss/agent/test/agent_hw_tests/AgentMmuTuningTests.cpp
  fboss/agent/test/agent_hw_tests/AgentSflowMirrorTest.cpp
  fboss/agent/test/agent_hw_tests/AgentAclPriorityTests.cpp
  fboss/agent/test/agent_hw_tests/AgentTrafficPauseTests.cpp
  fboss/agent/test/agent_hw_tests/AgentTrunkLoadBalancerTests.cpp
  fboss/agent/test/agent_hw_tests/AgentTrunkTests.cpp
  fboss/agent/test/agent_hw_tests/AgentRxReasonTests.cpp
  fboss/agent/test/agent_hw_tests/AgentRouteScaleTests.cpp
  fboss/agent/test/agent_hw_tests/AgentHwAclMatchActionsTest.cpp
  fboss/agent/test/agent_hw_tests/AgentHwAclQualifierTest.cpp
  fboss/agent/test/agent_hw_tests/AgentHwAclStatTests.cpp
  fboss/agent/test/agent_hw_tests/AgentHwResourceStatsTests.cpp
  fboss/agent/test/agent_hw_tests/AgentHwParityErrorTests.cpp
  fboss/agent/test/agent_hw_tests/AgentTrafficPfcTests.cpp
  fboss/agent/test/agent_hw_tests/AgentIpInIpTunnelTests.cpp
  fboss/agent/test/agent_hw_tests/AgentHwPtpTcTests.cpp
  fboss/agent/test/agent_hw_tests/AgentHwUdfTests.cpp
  fboss/agent/test/agent_hw_tests/AgentArsFlowletTest.cpp
  fboss/agent/test/agent_hw_tests/AgentRouterInterfaceCounterTest.cpp
)

target_link_libraries(agent_hw_test_src
  acl_test_utils
  address_utils
  agent_test_utils
  aqm_test_utils
  copp_test_utils
  dscp_marking_utils
  pkt_test_utils
  port_stats_test_utils
  packet
  packet_snooper
  queue_per_host_test_utils
  multi_port_traffic_test_utils
  trap_packet_utils
  core
  hw_asic_table
  hw_switch_fb303_stats
  hw_resource_stats_publisher
  config_factory
  agent_hw_test
  ecmp_helper
  ecmp_dataplane_test_util
  fabric_test_utils
  trunk_utils
  traffic_policy_utils
  olympic_qos_utils
  network_ai_qos_utils
  pfc_test_utils
  qos_test_utils
  queue_test_utils
  invariant_test_utils
  prod_config_factory
  port_test_utils
  udf_test_utils
  state
  stats
  route_scale_gen
  route_test_utils
  switch_asics
  trunk_test_utils
  resourcelibutil
  load_balancer_test_utils
  l2learn_observer_util
  mac_test_utils
  port_stats_test_utils
  utils
  sflow_shim_utils
  udf_test_utils
  pktutil
  mirror_test_utils
  dsf_config_utils
  voq_test_utils
  fib_updater
  ecmp_test_utils
  scale_test_utils
  neighbor_test_utils
  system_scale_test_utils
)

add_executable(multi_switch_agent_hw_test
  fboss/agent/test/agent_hw_tests/MultiSwitchAgentHwTest.cpp
)

target_link_libraries(multi_switch_agent_hw_test
  -Wl,--whole-archive
  acl_test_utils
  agent_test_utils
  copp_test_utils
  pkt_test_utils
  agent_hw_test_src
  agent_hw_test
  multi_switch_agent_ensemble
  olympic_qos_utils
  network_ai_qos_utils
  setup_thrift_prod
  trunk_utils
  traffic_policy_utils
  Folly::folly
  hw_packet_utils
  -Wl,--no-whole-archive
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

function(BUILD_SAI_AGENT_HW_TEST SAI_IMPL_NAME SAI_IMPL_ARG)

  message(STATUS "Building SAI_IMPL_NAME: ${SAI_IMPL_NAME} SAI_IMPL_ARG: ${SAI_IMPL_ARG}")
  add_executable(sai_agent_hw_test-${SAI_IMPL_NAME}
    fboss/agent/test/agent_hw_tests/SaiAgentHwTest.cpp
  )

  add_sai_sdk_dependencies(sai_agent_hw_test-${SAI_IMPL_NAME})

  target_link_libraries(sai_agent_hw_test-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    agent_hw_test_src
    ${SAI_IMPL_ARG}
    acl_test_utils
    copp_test_utils
    sai_acl_utils
    sai_copp_utils
    hw_packet_utils
    olympic_qos_utils
    network_ai_qos_utils
    traffic_policy_utils
    sai_traced_api
    setup_thrift_prod
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_agent_hw_test-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR} \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_library(agent_scale_test_src
    fboss/agent/test/agent_hw_tests/AgentAclScaleTests.cpp
    fboss/agent/test/agent_hw_tests/AgentEcmpScaleTests.cpp
  )

  add_sai_sdk_dependencies(agent_scale_test_src)

  target_link_libraries(agent_scale_test_src
    config_factory
    packet_factory
    agent_hw_test_src
    ecmp_helper
    production_features_cpp2
    acl_test_utils
    asic_test_utils
    scale_test_utils
    Folly::folly
  )

  add_executable(sai_agent_scale_test-${SAI_IMPL_NAME}
    fboss/agent/test/agent_hw_tests/SaiAgentHwTest.cpp
  )

  add_sai_sdk_dependencies(sai_agent_scale_test-${SAI_IMPL_NAME})

  target_link_libraries(sai_agent_scale_test-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    ${SAI_IMPL_ARG}
    agent_scale_test_src
    agent_hw_test
    sai_acl_utils
    mono_agent_ensemble
    agent_hw_test_thrift_handler
    setup_thrift_prod
    -Wl,--no-whole-archive
    ${GTEST}
    ${LIBGMOCK_LIBRARIES}
  )
endfunction()

if(BUILD_SAI_FAKE)
BUILD_SAI_AGENT_HW_TEST("fake" fake_sai)
install(
  TARGETS
  sai_agent_hw_test-fake)
endif()

# If libsai_impl is provided, build sai tests linking with it
find_library(SAI_IMPL sai_impl)
message(STATUS "SAI_IMPL: ${SAI_IMPL}")

if(SAI_IMPL)
  BUILD_SAI_AGENT_HW_TEST("sai_impl" ${SAI_IMPL})
  install(
    TARGETS
    sai_agent_hw_test-sai_impl)
endif()
