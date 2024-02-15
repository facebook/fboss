# CMake to build libraries and binaries in fboss/agent/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(acl_test_utils
  fboss/agent/test/utils/AclTestUtils.cpp
)

target_link_libraries(acl_test_utils
  fboss_error
  switch_config_cpp2
  switch_state_cpp2
)

add_library(copp_test_utils
  fboss/agent/test/utils/CoppTestUtils.cpp
)

target_link_libraries(copp_test_utils
  switch_asics
  packet_factory
  Folly::folly
  resourcelibutil
  switch_config_cpp2
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(pkt_test_utils
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
  ${GTEST}
)

add_library(traffic_policy_utils
  fboss/agent/test/utils/TrafficPolicyTestUtils.cpp
)

target_link_libraries(traffic_policy_utils
  switch_config_cpp2
  config_factory
  state
  Folly::folly
)

add_library(olympic_qos_utils
  fboss/agent/test/utils/OlympicTestUtils.cpp
)

target_link_libraries(olympic_qos_utils
  fboss_types
  packet_factory
  Folly::folly
  switch_config_cpp2
)

add_library(port_test_utils
  fboss/agent/test/utils/PortTestUtils.cpp
)

target_link_libraries(port_test_utils
  fboss_types
  Folly::folly
  switch_config_cpp2
  transceiver_cpp2
  state
)

add_library(config_utils
  fboss/agent/test/utils/ConfigUtils.cpp
)

target_link_libraries(config_utils
  agent_features
  fboss_types
  Folly::folly
  platform_mapping
  switch_config_cpp2
)

add_library(common_test_utils
  fboss/agent/test/utils/CommonUtils.cpp
)

target_link_libraries(common_test_utils
  core
  switch_asics
)
