# CMake to build test binaries in fboss/cli/fboss2/test/integration_test

# fboss2_integration_test - CLI E2E test binary
#
# CLI tests are platform/SAI independent - they test the CLI binary which
# communicates with the agent via Thrift, without running the actual fboss2-dev
# binary.
add_executable(fboss2_integration_test
  fboss/cli/fboss2/oss/config/CmdListImpl.cpp
  fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceDescriptionTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceIpv6NdTest.cpp
  fboss/cli/fboss2/test/integration_test/DeleteInterfaceIpv6NdTest.cpp
  fboss/cli/fboss2/test/integration_test/DeleteInterfaceTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceMtuTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceProfileTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceFlowControlTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceLldpExpectedValueTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceLoopbackModeTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceTypeTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigQosPolicyMapTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigSessionClearTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceSwitchportTrunkAllowedVlanTest.cpp
  fboss/cli/fboss2/utils/CmdInitUtils.cpp
  fboss/cli/fboss2/utils/oss/CmdInitUtils.cpp
)

target_link_libraries(fboss2_integration_test
  fboss2_lib
  fboss2_config_lib
  thrift_service_utils
  CLI11::CLI11
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  Folly::folly
  Folly::folly_test_util
  FBThrift::thriftcpp2
  gflags
  fmt::fmt
)

target_include_directories(fboss2_integration_test PUBLIC
  ${CMAKE_SOURCE_DIR}
)
