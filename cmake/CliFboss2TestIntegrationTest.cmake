# CMake to build test binaries in fboss/cli/fboss2/test/integration_test

# fboss2_integration_test - CLI E2E test binary
#
# These tests require a live FBOSS agent running on the device and should
# only be run on actual hardware, not in CI. The binary name includes "hw_test"
# so it will be automatically skipped by the GitHub Actions test runner.
add_executable(fboss2_integration_test
  fboss/cli/fboss2/oss/CmdListConfig.cpp
  fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceDescriptionTest.cpp
  fboss/cli/fboss2/test/integration_test/ConfigInterfaceMtuTest.cpp
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
