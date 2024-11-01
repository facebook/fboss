# CMake to build libraries and binaries in fboss/agent/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(label_forwarding_utils
  fboss/agent/test/LabelForwardingUtils.cpp
)

target_link_libraries(label_forwarding_utils
  state
)

add_library(route_distribution_gen
  fboss/agent/test/RouteDistributionGenerator.cpp
)

add_library(resourcelibutil
  fboss/agent/test/ResourceLibUtil.cpp
)

target_link_libraries(resourcelibutil
  state
)

target_link_libraries(route_distribution_gen
  ecmp_helper
  resourcelibutil
  state
)

add_library(route_scale_gen
  fboss/agent/test/RouteScaleGenerators.cpp
)

target_link_libraries(route_scale_gen
  ecmp_helper
  route_distribution_gen
  state
)

add_library(agent_test_utils
  fboss/agent/test/CounterCache.cpp
  fboss/agent/test/MockTunManager.cpp
  fboss/agent/test/TestUtils.cpp
)

target_link_libraries(agent_test_utils
  core
  label_forwarding_utils
  hw_mock
  monolithic_switch_handler
  multi_switch_hw_switch_handler
)

add_library(ecmp_helper
  fboss/agent/test/EcmpSetupHelper.cpp
)

target_link_libraries(ecmp_helper
  switch_config_cpp2
  state
  core
)

add_library(trunk_utils
  fboss/agent/test/TrunkUtils.cpp
)

target_link_libraries(trunk_utils
  switch_config_cpp2
  state
)

add_executable(async_logger_test
  fboss/agent/test/oss/Main.cpp
  fboss/agent/test/AsyncLoggerTest.cpp
)

target_link_libraries(async_logger_test
  async_logger
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(async_logger_test)

add_library(agent_test_lib
  fboss/agent/test/AgentTest.cpp
)

target_link_libraries(agent_test_lib
  main
  qsfp_cpp2
  qsfp_service_client
  fboss_config_utils
  monolithic_agent_initializer
  qos_test_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(agent_ensemble_test_lib
  fboss/agent/test/AgentEnsembleTest.cpp
)

target_link_libraries(agent_ensemble_test_lib
  main
  qsfp_cpp2
  qsfp_service_client
  fboss_config_utils
  agent_ensemble
  agent_features
  qos_test_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(agent_integration_test_base
  fboss/agent/test/AgentIntegrationTestBase.cpp
)

target_link_libraries(agent_integration_test_base
  agent_test_lib
  main
  config_factory
  fboss_config_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(agent_hw_test
  fboss/agent/test/AgentHwTest.cpp
)

target_link_libraries(agent_hw_test
  agent_hw_test_constants
  mono_agent_ensemble
  production_features_cpp2
  core
  switch_asics
  hw_copp_utils
  stats_test_utils
  hardware_stats_cpp2
  ${GTEST}
)

add_library(multinode_tests
  fboss/agent/test/MultiNodeTest.cpp
  fboss/agent/test/MultiNodeLacpTests.cpp
  fboss/agent/test/MultiNodeLoadBalancerTests.cpp
)

target_link_libraries(multinode_tests
  agent_test_lib
  main
  config_factory
  trunk_utils
  fboss_config_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(test_ensemble_if
  fboss/agent/test/TestEnsembleIf.cpp
)

target_link_libraries(test_ensemble_if
  state
  hw_switch
)

add_library(agent_ensemble
  fboss/agent/test/AgentEnsemble.cpp
)

target_link_libraries(agent_ensemble
  handler
  hw_link_state_toggler
  route_distribution_gen
  main
  config_factory
  fboss_config_utils
  test_ensemble_if
  pkt_test_utils
  agent_hw_test_ctrl_cpp2
  FBThrift::thriftcpp2
  ${GTEST}
)

add_library(mono_agent_ensemble
  fboss/agent/test/MonoAgentEnsemble.cpp
)

target_link_libraries(mono_agent_ensemble
  agent_ensemble
  monolithic_agent_initializer
  agent_hw_test_thrift_handler
  ${GTEST}
)

add_library(multi_switch_agent_ensemble
  fboss/agent/test/MultiSwitchAgentEnsemble.cpp
)

target_link_libraries(multi_switch_agent_ensemble
  agent_ensemble
  split_agent_initializer
  ${GTEST}
)

add_library(linkstate_toggler
  fboss/agent/test/LinkStateToggler.cpp
)

target_link_libraries(linkstate_toggler
  state
  core
)
