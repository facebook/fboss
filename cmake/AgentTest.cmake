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
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(agent_hw_test_lib
  fboss/agent/test/AgentHwTest.cpp
)

target_link_libraries(agent_hw_test_lib
  agent_test_lib
  main
  fboss_agent
  config_factory
  fboss_config_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(multinode_tests
  fboss/agent/test/MultiNodeTest.cpp
  fboss/agent/test/MultiNodeLacpTests.cpp
  fboss/agent/test/MultiNodeLoadBalancerTests.cpp
)

target_link_libraries(multinode_tests
  agent_test_lib
  main
  fboss_agent
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
)

add_library(agent_ensemble
  fboss/agent/test/AgentEnsemble.cpp
)

target_link_libraries(agent_ensemble
  route_distribution_gen
  main
  fboss_agent
  config_factory
  fboss_config_utils
  test_ensemble_if
)
