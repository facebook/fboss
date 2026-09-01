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
#include <map>
#include <stdexcept>
#include <string>

#include <CLI/App.hpp>
#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"
#include "fboss/cli/fboss2/CmdList.h"
#include "fboss/cli/fboss2/CmdSubcommands.h"
#include "fboss/cli/fboss2/commands/config/gen/PlatformConfigPathUtils.h"
#include "fboss/cli/fboss2/utils/CLIParserUtils.h"

namespace facebook::fboss::configgen {
namespace {

namespace fs = std::filesystem;

constexpr std::string_view kPlatform = "test_platform";
constexpr std::string_view kProfile = "hw_test";
constexpr std::string_view kAsicYaml = "ASIC_CONFIG: test\n";
constexpr std::string_view kAsicJson = "{\"ASIC_CONFIG\":\"test\"}\n";
constexpr std::string_view kKeyValueConfig =
    "{\"foo\":\"bar\",\"answer\":\"42\"}\n";

void writeTestFile(const fs::path& path, std::string_view contents) {
  fs::create_directories(path.parent_path());
  if (!folly::writeFile(contents, path.c_str())) {
    throw std::runtime_error("Unable to write test file " + path.string());
  }
}

fs::path createTestPlatform(
    const fs::path& fbossRoot,
    std::string_view vendor,
    std::string_view platform = kPlatform,
    std::string_view configType = "YAML_CONFIG",
    std::string_view extension = ".yml",
    std::string_view generatedConfig = kAsicYaml) {
  const auto asicConfigDirectory =
      fbossRoot / "configs" / "platforms" / vendor / platform / "asic_config";
  writeTestFile(
      asicConfigDirectory / "asic_config.json",
      "{\"platform_name\":\"" + std::string(platform) +
          "\",\"defaults\":{\"asic_config_params\":{\"config_type\":\"" +
          std::string(configType) +
          "\"}},\"variants\":{\"\":{},\"hw_test\":{}}}\n");
  writeTestFile(
      asicConfigDirectory / "generated" /
          (std::string(platform) + std::string(extension)),
      generatedConfig);
  writeTestFile(
      asicConfigDirectory / "generated" /
          (std::string(platform) + "_hw_test" + std::string(extension)),
      generatedConfig);
  return asicConfigDirectory.parent_path();
}

std::string readFile(const fs::path& path) {
  std::string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw std::runtime_error("Unable to read test file " + path.string());
  }
  return contents;
}

TEST(PlatformConfigPathUtilsTest, FindsPlatformAndComponentDirectories) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  const auto expectedPlatformDirectory =
      createTestPlatform(fbossRoot, "test_vendor");
  fs::create_directories(expectedPlatformDirectory / "platform_mapping");

  const auto platformDirectory =
      findPlatformConfigDirectory(fbossRoot, kPlatform);

  EXPECT_EQ(platformDirectory.systemVendor, "test_vendor");
  EXPECT_EQ(platformDirectory.path, expectedPlatformDirectory);
  EXPECT_EQ(
      findPlatformConfigComponentDirectory(
          platformDirectory, "platform_mapping"),
      expectedPlatformDirectory / "platform_mapping");
}

TEST(PlatformConfigPathUtilsTest, RejectsDuplicatePlatformNames) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(fbossRoot, "first_vendor");
  createTestPlatform(fbossRoot, "second_vendor");

  EXPECT_THROW(findPlatformConfigDirectory(fbossRoot, kPlatform), FbossError);
}

TEST(AgentConfigGenTest, AssemblesEmptyAgentConfig) {
  auto config = assembleAgentConfig({}, {}, {});

  EXPECT_TRUE(config.defaultCommandLineArgs()->empty());
  EXPECT_EQ(*config.sw(), cfg::SwitchConfig());
  EXPECT_EQ(*config.platform(), cfg::PlatformConfig());
  EXPECT_TRUE(config.thriftApiToRateLimitInQps()->empty());
}

TEST(AgentConfigGenTest, GeneratesPlatformConfigFromProvidedSections) {
  cfg::AsicConfigEntry common;
  common.set_yamlConfig("asic config");
  cfg::AsicConfig asicConfig;
  asicConfig.common() = std::move(common);
  cfg::ChipConfig chipConfig;
  chipConfig.set_asicConfig(std::move(asicConfig));

  cfg::PortAssignment assignment;
  assignment.portName() = "eth1/1/1";
  assignment.portType() = cfg::PortType::INTERFACE_PORT;
  assignment.scope() = cfg::Scope::LOCAL;
  std::map<int32_t, cfg::PortAssignment> assignments{
      {1, std::move(assignment)}};

  const auto platformConfig = generatePlatformConfig(chipConfig, assignments);

  EXPECT_EQ(*platformConfig.chip(), chipConfig);
  EXPECT_EQ(*platformConfig.portIdToPortAssignment(), assignments);
}

TEST(AgentConfigGenTest, SelectsDefaultOrProfiledGeneratedAsicConfig) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  const auto platformDirectory = createTestPlatform(fbossRoot, "test_vendor");

  EXPECT_EQ(
      findGeneratedAsicConfig(fbossRoot, kPlatform, kProfile),
      platformDirectory / "asic_config" / "generated" /
          "test_platform_hw_test.yml");
  EXPECT_EQ(
      findGeneratedAsicConfig(fbossRoot, kPlatform, ""),
      platformDirectory / "asic_config" / "generated" / "test_platform.yml");
  EXPECT_EQ(
      findGeneratedAsicConfig(fbossRoot, kPlatform, "default"),
      platformDirectory / "asic_config" / "generated" / "test_platform.yml");
}

TEST(AgentConfigGenTest, GeneratesAsicOnlyPlatformConfig) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(fbossRoot, "test_vendor");

  const auto platformConfig =
      generatePlatformConfigFromAsicConfig(fbossRoot, kPlatform, kProfile);

  ASSERT_EQ(
      platformConfig.chip()->getType(), cfg::ChipConfig::Type::asicConfig);
  EXPECT_EQ(
      platformConfig.chip()->get_asicConfig().common()->get_yamlConfig(),
      kAsicYaml);
  EXPECT_TRUE(platformConfig.portIdToPortAssignment()->empty());
  EXPECT_FALSE(platformConfig.platformSettings());
}

TEST(AgentConfigGenTest, GeneratesJsonAsicConfig) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(
      fbossRoot, "test_vendor", kPlatform, "JSON_CONFIG", ".json", kAsicJson);

  const auto platformConfig =
      generatePlatformConfigFromAsicConfig(fbossRoot, kPlatform, kProfile);
  const auto& common = *platformConfig.chip()->get_asicConfig().common();

  ASSERT_EQ(common.getType(), cfg::AsicConfigEntry::Type::jsonConfig);
  EXPECT_EQ(common.get_jsonConfig(), kAsicJson);
}

TEST(AgentConfigGenTest, GeneratesKeyValueAsicConfig) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(
      fbossRoot,
      "test_vendor",
      kPlatform,
      "KEY_VALUE_CONFIG",
      ".json",
      kKeyValueConfig);

  const auto platformConfig =
      generatePlatformConfigFromAsicConfig(fbossRoot, kPlatform, kProfile);
  const auto& common = *platformConfig.chip()->get_asicConfig().common();
  const std::map<std::string, std::string> expected{
      {"answer", "42"}, {"foo", "bar"}};

  ASSERT_EQ(common.getType(), cfg::AsicConfigEntry::Type::config);
  EXPECT_EQ(common.get_config(), expected);
}

TEST(AgentConfigGenTest, RejectsUnsupportedAsicConfigType) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(
      fbossRoot, "test_vendor", kPlatform, "UNSUPPORTED_CONFIG", ".json", "{}");

  EXPECT_THROW(
      generatePlatformConfigFromAsicConfig(fbossRoot, kPlatform, kProfile),
      FbossError);
}

TEST(AgentConfigGenTest, SerializesAndWritesAgentConfig) {
  folly::test::TemporaryDirectory sourceDirectory;
  folly::test::TemporaryDirectory outputDirectory;
  const auto fbossRoot = fs::path(sourceDirectory.path().string()) / "fboss";
  createTestPlatform(fbossRoot, "test_vendor");

  auto outputPath = generateAgentConfig(
      kPlatform,
      kProfile,
      fbossRoot,
      fs::path(outputDirectory.path().string()));

  EXPECT_EQ(outputPath.filename(), "agent.conf");
  cfg::AgentConfig config;
  apache::thrift::SimpleJSONSerializer::deserialize(
      readFile(outputPath), config);
  EXPECT_TRUE(config.defaultCommandLineArgs()->empty());
  EXPECT_EQ(*config.sw(), cfg::SwitchConfig());
  EXPECT_EQ(
      *config.platform(),
      generatePlatformConfigFromAsicConfig(fbossRoot, kPlatform, kProfile));
  EXPECT_TRUE(config.thriftApiToRateLimitInQps()->empty());
}

TEST(AgentConfigGenTest, UsesUniqueDefaultOutputDirectory) {
  folly::test::TemporaryDirectory sourceDirectory;
  const auto fbossRoot = fs::path(sourceDirectory.path().string()) / "fboss";
  createTestPlatform(fbossRoot, "test_vendor");

  auto outputPath = generateAgentConfig(kPlatform, kProfile, fbossRoot);

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
  folly::test::TemporaryDirectory sourceDirectory;
  folly::test::TemporaryDirectory outputDirectory;
  const auto fbossRoot = fs::path(sourceDirectory.path().string()) / "fboss";
  const auto outputRoot = fs::path(outputDirectory.path().string());
  createTestPlatform(fbossRoot, "test_vendor");
  auto outputPath =
      generateAgentConfig(kPlatform, kProfile, fbossRoot, outputRoot);

  EXPECT_THROW(
      generateAgentConfig(kPlatform, kProfile, fbossRoot, outputRoot),
      std::system_error);

  cfg::AgentConfig config;
  EXPECT_NO_THROW(
      apache::thrift::SimpleJSONSerializer::deserialize(
          readFile(outputPath), config));
}

TEST(AgentConfigGenTest, RejectsUnknownPlatformOrProfile) {
  folly::test::TemporaryDirectory sourceDirectory;
  folly::test::TemporaryDirectory outputDirectory;
  const auto fbossRoot = fs::path(sourceDirectory.path().string()) / "fboss";
  const auto outputRoot = fs::path(outputDirectory.path().string());
  createTestPlatform(fbossRoot, "test_vendor");

  EXPECT_THROW(
      generateAgentConfig("unknown", kProfile, fbossRoot, outputRoot),
      FbossError);
  EXPECT_THROW(
      generateAgentConfig(kPlatform, "unknown", fbossRoot, outputRoot),
      FbossError);
  EXPECT_TRUE(fs::is_empty(outputRoot));
}

TEST(AgentConfigGenTest, RejectsActiveConfigDirectory) {
  folly::test::TemporaryDirectory sourceDirectory;
  const auto fbossRoot = fs::path(sourceDirectory.path().string()) / "fboss";
  createTestPlatform(fbossRoot, "test_vendor");

  EXPECT_THROW(
      generateAgentConfig(
          kPlatform,
          kProfile,
          fbossRoot,
          fs::path("/etc/coop/config-gen-test")),
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
