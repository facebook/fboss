# CMake to build test binaries in fboss/cli/fboss2/test/config
# Corresponds to BUCK target: fboss/cli/fboss2/test/config:cmd_config_test

add_executable(fboss2_cmd_config_test
  fboss/cli/fboss2/oss/CmdListConfig.cpp
  fboss/cli/fboss2/test/TestMain.cpp
  fboss/cli/fboss2/test/config/CmdConfigAppliedInfoTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigHistoryTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigInterfaceDescriptionTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigInterfaceMtuTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigInterfaceSwitchportAccessVlanTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigQosBufferPoolTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigReloadTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigSessionDiffTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigSessionTest.cpp
  fboss/cli/fboss2/test/config/CmdConfigTestBase.cpp
)

target_link_libraries(fboss2_cmd_config_test
  fboss2_lib
  fboss2_config_lib
  thrift_service_utils
  Boost::filesystem
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  Folly::folly
  Folly::follybenchmark
  Folly::folly_test_util
  FBThrift::thriftcpp2
)

gtest_discover_tests(fboss2_cmd_config_test)
