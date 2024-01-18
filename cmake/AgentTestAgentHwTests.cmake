# CMake to build libraries and binaries in fboss/agent/hw/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(agent_hw_test_src
  fboss/agent/test/agent_hw_tests/AgentEmptyTests.cpp
)

target_link_libraries(agent_hw_test_src
  core
  hw_switch_fb303_stats
  config_factory
  agent_hw_test
  ecmp_helper
)

add_executable(multi_switch_agent_hw_test
  fboss/agent/test/agent_hw_tests/MultiSwitchAgentHwTest.cpp
)

target_link_libraries(multi_switch_agent_hw_test
  -Wl,--whole-archive
  agent_hw_test_src
  agent_hw_test
  multi_switch_agent_ensemble
  Folly::folly
  -Wl,--no-whole-archive
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
