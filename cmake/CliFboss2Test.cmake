# CMake to build test binaries in fboss/cli/fboss2/test

# cmd_test - Command tests from BUCK file
add_executable(fboss2_cmd_test
  fboss/cli/fboss2/test/TestMain.cpp
  fboss/cli/fboss2/test/CmdConfigAppliedInfoTest.cpp
  fboss/cli/fboss2/test/CmdConfigReloadTest.cpp
  fboss/cli/fboss2/test/CmdConfigSessionDiffTest.cpp
  fboss/cli/fboss2/test/CmdConfigSessionTest.cpp
  fboss/cli/fboss2/test/CmdSetPortStateTest.cpp
  fboss/cli/fboss2/test/CmdShowAclTest.cpp
  fboss/cli/fboss2/test/CmdShowAgentSslTest.cpp
  fboss/cli/fboss2/test/CmdShowArpTest.cpp
  # fboss/cli/fboss2/test/CmdShowHardwareTest.cpp - excluded (hardware model not built in CMake)
  fboss/cli/fboss2/test/CmdShowInterfaceStatusTest.cpp
  fboss/cli/fboss2/test/CmdShowInterfaceTest.cpp
  fboss/cli/fboss2/test/CmdShowInterfaceTrafficTest.cpp
  fboss/cli/fboss2/test/CmdShowL2Test.cpp
  fboss/cli/fboss2/test/CmdShowLldpTest.cpp
  fboss/cli/fboss2/test/CmdShowNdpTest.cpp
  fboss/cli/fboss2/test/CmdGetPcapTest.cpp
  fboss/cli/fboss2/test/CmdShowAggregatePortTest.cpp
  fboss/cli/fboss2/test/CmdShowCpuPortTest.cpp
  fboss/cli/fboss2/test/CmdShowExampleTest.cpp
  fboss/cli/fboss2/test/CmdShowFlowletTest.cpp
  fboss/cli/fboss2/test/CmdShowHostTest.cpp
  fboss/cli/fboss2/test/CmdShowHwAgentStatusTest.cpp
  fboss/cli/fboss2/test/CmdShowHwObjectTest.cpp
  fboss/cli/fboss2/test/CmdShowInterfaceCountersTest.cpp
  fboss/cli/fboss2/test/CmdShowInterfaceErrorsTest.cpp
  # fboss/cli/fboss2/test/CmdShowInterfaceFlapsTest.cpp - excluded (depends on hardware model not built in CMake)
  fboss/cli/fboss2/test/CmdShowMirrorTest.cpp
  fboss/cli/fboss2/test/CmdShowPortTest.cpp
  fboss/cli/fboss2/test/CmdShowProductDetailsTest.cpp
  fboss/cli/fboss2/test/CmdShowProductTest.cpp
  fboss/cli/fboss2/test/CmdShowRouteDetailsTest.cpp
  fboss/cli/fboss2/test/CmdShowRouteSummaryTest.cpp
  fboss/cli/fboss2/test/CmdShowTeFlowTest.cpp
  # fboss/cli/fboss2/test/CmdShowTransceiverTest.cpp - excluded (depends on configerator bgp namespace)
  fboss/cli/fboss2/test/CmdStartPcapTest.cpp
  fboss/cli/fboss2/test/CmdStopPcapTest.cpp
  fboss/cli/fboss2/test/PortMapTest.cpp
)

target_link_libraries(fboss2_cmd_test
  fboss2_lib
  fboss2_config_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  Folly::folly
  Folly::follybenchmark
  Folly::folly_test_util
  FBThrift::thriftcpp2
)

gtest_discover_tests(fboss2_cmd_test)
