# CMake to build libraries and binaries in fboss/agent/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(agent_test_acl_utils
  fboss/agent/test/utils/AgentTestAclUtils.cpp
)

target_link_libraries(agent_test_acl_utils
  fboss_error
  switch_config_cpp2
  switch_state_cpp2
)
