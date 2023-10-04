// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

// @lint-ignore CLANGTIDY
#include "fboss/agent/gen-cpp2-thriftpath/agent_config.h" // @manual=//fboss/agent:agent_config-cpp2-thriftpath
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath

TEST(PathTests, toStrSimple) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<TestStruct> root;

  EXPECT_EQ(root.tx().str(), "/tx");
  EXPECT_EQ(root.structMap()[3].max().str(), "/structMap/3/max");
  EXPECT_EQ(root.member().min().str(), "/member/min");
  EXPECT_EQ(root.str(), "/");
}

TEST(PathTests, toStrAgentConfig) {
  using namespace facebook::fboss::fsdb;

  thriftpath::RootThriftPath<facebook::fboss::cfg::AgentConfig> root;

  EXPECT_EQ(root.defaultCommandLineArgs().str(), "/defaultCommandLineArgs");
  EXPECT_EQ(
      root.defaultCommandLineArgs()["bla"].str(),
      "/defaultCommandLineArgs/bla");
  EXPECT_EQ(
      root.sw().portQueueConfigs()["testQueue"].str(),
      "/sw/portQueueConfigs/testQueue");
  EXPECT_EQ(
      root.platform().chip().bcm().config()["plp_freq"].str(),
      "/platform/chip/bcm/config/plp_freq");
  EXPECT_EQ(
      root.platform().chip().asic().config().str(),
      "/platform/chip/asic/config");
  EXPECT_EQ(root.str(), "/");
}

TEST(PathTests, IntegralGetOperator) {
  using namespace facebook::fboss::fsdb;
  using k = facebook::fboss::cfg::agent_config_tags::strings;
  using AgentConfigMembers =
      apache::thrift::reflect_struct<facebook::fboss::cfg::AgentConfig>::member;

  using RootPath =
      thriftpath::RootThriftPath<facebook::fboss::cfg::AgentConfig>;
  RootPath root;

  EXPECT_EQ(
      root(AgentConfigMembers::defaultCommandLineArgs::id()).str(),
      "/defaultCommandLineArgs");
  EXPECT_EQ(root(k::defaultCommandLineArgs()).str(), "/defaultCommandLineArgs");
}

TEST(PathTests, AgentConfigTokens) {
  using namespace facebook::fboss::fsdb;

  using RootPath =
      thriftpath::RootThriftPath<facebook::fboss::cfg::AgentConfig>;
  RootPath root;

  using PathVec = std::vector<std::string>;
  auto cmdlineArgs = root.defaultCommandLineArgs();
  EXPECT_EQ(cmdlineArgs.tokens(), PathVec({"defaultCommandLineArgs"}));
  EXPECT_EQ(cmdlineArgs.idTokens(), PathVec({"1"}));
  EXPECT_EQ(
      cmdlineArgs["bla"].tokens(), PathVec({"defaultCommandLineArgs", "bla"}));
  EXPECT_EQ(cmdlineArgs["bla"].idTokens(), PathVec({"1", "bla"}));
  auto portQueueConfigs = root.sw().portQueueConfigs();
  EXPECT_EQ(portQueueConfigs.tokens(), PathVec({"sw", "portQueueConfigs"}));
  EXPECT_EQ(portQueueConfigs.idTokens(), PathVec({"2", "40"}));

  // Test rvalue ref child getters
  EXPECT_EQ(
      RootPath().defaultCommandLineArgs()["foo"].tokens(),
      PathVec({"defaultCommandLineArgs", "foo"}));
  EXPECT_EQ(
      RootPath().defaultCommandLineArgs()["foo"].idTokens(),
      PathVec({"1", "foo"}));
}
