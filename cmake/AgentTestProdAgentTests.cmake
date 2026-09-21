# CMake to build libraries and binaries in fboss/agent/hw/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake


add_library(invariant_agent_tests
  fboss/agent/test/prod_agent_tests/ProdAgentTests.cpp
  fboss/agent/test/prod_agent_tests/ProdInvariantTests.cpp
)


target_link_libraries(invariant_agent_tests
  Folly::folly
  agent_features
  agent_features
  core
  handler
  load_agent_config
  main
  route_update_wrapper
  setup_thrift
  config_factory
  hw_copp_utils
  hw_packet_utils
  hw_qos_utils
  load_balancer_utils
  prod_config_factory
  state
  agent_ensemble_test_lib
  ecmp_helper
  acl_test_utils
  dscp_marking_utils
  queue_per_host_test_utils
  fboss_config_utils
  invariant_test_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
