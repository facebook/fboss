/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/gen/agent/AgentConfigGenUtils.h"

#include <filesystem>
#include <stdexcept>

#include <CLI/App.hpp>
#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/cli/fboss2/CmdList.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"
#include "fboss/cli/fboss2/utils/CLIParserUtils.h"

namespace facebook::fboss::configgen {
namespace {

namespace fs = std::filesystem;

TEST(AgentConfigGenTest, AssemblesEmptyAgentConfig) {
  auto config = assembleAgentConfig({}, {}, {});

  EXPECT_TRUE(config.defaultCommandLineArgs()->empty());
  EXPECT_EQ(*config.sw(), cfg::SwitchConfig());
  EXPECT_EQ(*config.platform(), cfg::PlatformConfig());
  EXPECT_TRUE(config.thriftApiToRateLimitInQps()->empty());
}

TEST(AgentConfigGenTest, SerializesAndWritesAgentConfig) {
  folly::test::TemporaryDirectory tempDirectory;
  auto outputPath = generateAgentConfig(
      "wedge800bact", "hw-test", fs::path(tempDirectory.path().string()));

  EXPECT_EQ(outputPath.filename(), "agent.conf");
  std::string contents;
  ASSERT_TRUE(folly::readFile(outputPath.c_str(), contents));

  cfg::AgentConfig config;
  apache::thrift::SimpleJSONSerializer::deserialize(contents, config);
  EXPECT_TRUE(config.defaultCommandLineArgs()->empty());
  EXPECT_EQ(*config.sw(), cfg::SwitchConfig());
  EXPECT_EQ(*config.platform(), cfg::PlatformConfig());
  EXPECT_TRUE(config.thriftApiToRateLimitInQps()->empty());
}

TEST(AgentConfigGenTest, UsesUniqueDefaultOutputDirectory) {
  auto outputPath = generateAgentConfig("wedge800bact", "hw-test");

  EXPECT_EQ(
      outputPath.parent_path().parent_path(),
      fs::path("/tmp/fboss2/config-gen"));
  EXPECT_EQ(outputPath.filename(), "agent.conf");
  EXPECT_TRUE(fs::is_regular_file(outputPath));

  std::error_code error;
  fs::remove_all(outputPath.parent_path(), error);
  EXPECT_FALSE(error);
}

TEST(AgentConfigGenTest, DoesNotOverwriteExistingConfig) {
  folly::test::TemporaryDirectory tempDirectory;
  auto outputDirectory = fs::path(tempDirectory.path().string());
  auto outputPath =
      generateAgentConfig("wedge800bact", "hw-test", outputDirectory);

  EXPECT_THROW(
      generateAgentConfig("wedge800bact", "hw-test", outputDirectory),
      std::system_error);

  std::string contents;
  ASSERT_TRUE(folly::readFile(outputPath.c_str(), contents));
  cfg::AgentConfig config;
  EXPECT_NO_THROW(
      apache::thrift::SimpleJSONSerializer::deserialize(contents, config));
}

TEST(AgentConfigGenTest, AcceptsOtherSelectors) {
  folly::test::TemporaryDirectory tempDirectory;
  auto outputDirectory = fs::path(tempDirectory.path().string());

  EXPECT_NO_THROW(
      generateAgentConfig("montblanc", "benchmark", outputDirectory));
}

TEST(AgentConfigGenTest, RejectsActiveConfigDirectory) {
  EXPECT_THROW(
      generateAgentConfig(
          "wedge800bact", "hw-test", fs::path("/etc/coop/config-gen-test")),
      std::invalid_argument);
}

TEST(CmdConfigGenAgentTest, RegistersAgentCommand) {
  CLI::App app{"Test CLI"};
  EXPECT_NO_THROW(CmdSubcommands().init(app, kCommandTree(), {}, {}));

  auto* config = utils::getSubcommandIf(app, "config");
  ASSERT_NE(config, nullptr);
  auto* gen = utils::getSubcommandIf(*config, "gen");
  ASSERT_NE(gen, nullptr);
  EXPECT_NE(utils::getSubcommandIf(*gen, "agent"), nullptr);
}

} // namespace
} // namespace facebook::fboss::configgen
