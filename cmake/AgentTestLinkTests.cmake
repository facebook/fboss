# CMake to build libraries and binaries for agent link testing

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(link_tests
  fboss/agent/test/link_tests/EmptyLinkTest.cpp
  fboss/agent/test/link_tests/LinkTest.cpp
  fboss/agent/test/link_tests/LinkTestUtils.cpp
  fboss/agent/test/link_tests/LinkSanityTests.cpp
  fboss/agent/test/link_tests/PtpTests.cpp
  fboss/agent/test/link_tests/OpticsTest.cpp
)

target_link_libraries(link_tests
  agent_test_lib
  ecmp_helper
  main
  config_factory
  fboss_config_utils
  load_balancer_test_utils
  load_balancer_utils
  olympic_qos_utils
  port_test_utils
  copp_test_utils
  packet_snooper
  hw_packet_utils
  packet
  packet_snooper
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(agent_ensemble_link_tests
  fboss/agent/test/link_tests/AgentEnsembleLinkTest.cpp
  fboss/agent/test/link_tests/LinkTestUtils.cpp
  fboss/agent/test/link_tests/AgentEnsembleOpticsTest.cpp
  fboss/agent/test/link_tests/AgentEnsembleEmptyLinkTest.cpp
)

target_link_libraries(agent_ensemble_link_tests
  agent_ensemble_test_lib
  ecmp_helper
  main
  config_factory
  fboss_config_utils
  load_balancer_test_utils
  load_balancer_utils
  olympic_qos_utils
  port_test_utils
  copp_test_utils
  packet_snooper
  hw_packet_utils
  packet
  packet_snooper
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
