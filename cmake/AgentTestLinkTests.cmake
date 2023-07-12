# CMake to build libraries and binaries for agent link testing

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(link_tests
  fboss/agent/test/link_tests/EmptyLinkTest.cpp
  fboss/agent/test/link_tests/LinkTest.cpp
  fboss/agent/test/link_tests/LinkTestUtils.cpp
  fboss/agent/test/link_tests/LinkSanityTests.cpp
  fboss/agent/test/link_tests/PtpTests.cpp
)

target_link_libraries(link_tests
  agent_test_lib
  ecmp_helper
  main
  config_factory
  fboss_config_utils
  load_balancer_utils
  hw_olympic_qos_utils
  hw_agent_packet_utils
  packet
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
