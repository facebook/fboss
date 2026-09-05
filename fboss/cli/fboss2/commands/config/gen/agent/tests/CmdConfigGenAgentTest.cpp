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
constexpr std::string_view kPortName = "eth1/1/1";

void writeTestFile(const fs::path& path, std::string_view contents) {
  fs::create_directories(path.parent_path());
  if (!folly::writeFile(contents, path.c_str())) {
    throw std::runtime_error("Unable to write test file " + path.string());
  }
}

void writePortAssignments(
    const fs::path& path,
    int32_t portId,
    std::string_view portName) {
  writeTestFile(
      path,
      "{\"portIdToPortAssignment\":{\"" + std::to_string(portId) +
          "\":{\"portName\":\"" + std::string(portName) +
          "\",\"portType\":0,\"scope\":0}}}\n");
}

void writePlatformDescriptor(const fs::path& path, int16_t numSwitchAsics) {
  PlatformDescriptor descriptor;
  descriptor.platformType() = PlatformType::PLATFORM_WEDGE800BACT;
  descriptor.productNamePrefixes() = {"TestPlatform"};
  descriptor.modeNames() = {std::string(kPlatform)};
  descriptor.asicType() = cfg::AsicType::ASIC_TYPE_TOMAHAWK5;
  descriptor.numSwitchAsics() = numSwitchAsics;
  writeTestFile(
      path,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(descriptor));
}

std::map<int32_t, cfg::PortAssignment> makePortAssignments(
    int32_t portId,
    std::string_view portName) {
  cfg::PortAssignment assignment;
  assignment.portName() = portName;
  assignment.portType() = cfg::PortType::INTERFACE_PORT;
  assignment.scope() = cfg::Scope::LOCAL;
  return {{portId, std::move(assignment)}};
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
  writePortAssignments(
      fbossRoot / "lib" / "platform_mapping_v2" /
          "generated_platform_mappings" / vendor / platform /
          "port_id_to_port_assignment.json",
      1,
      kPortName);
  writePlatformDescriptor(
      fbossRoot / "lib" / "platform_mapping_v2" /
          "generated_platform_mappings" / vendor / platform /
          "platform_descriptor.json",
      1);
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

TEST(AgentConfigGenTest, SelectsGeneratedPlatformArtifacts) {
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
  EXPECT_EQ(
      findPortIdToPortAssignmentConfig(fbossRoot, kPlatform),
      fbossRoot / "lib" / "platform_mapping_v2" /
          "generated_platform_mappings" / "test_vendor" / kPlatform /
          "port_id_to_port_assignment.json");
  const auto [descriptorPath, descriptor] =
      findPlatformDescriptorConfigWithDescriptor(fbossRoot, kPlatform);
  EXPECT_EQ(
      descriptorPath,
      fbossRoot / "lib" / "platform_mapping_v2" /
          "generated_platform_mappings" / "test_vendor" / kPlatform /
          "platform_descriptor.json");
  EXPECT_EQ(*descriptor.asicType(), cfg::AsicType::ASIC_TYPE_TOMAHAWK5);
}

TEST(AgentConfigGenTest, GeneratesSingleNpuSwitchSettings) {
  PlatformDescriptor descriptor;
  descriptor.asicType() = cfg::AsicType::ASIC_TYPE_TOMAHAWK5;
  descriptor.numSwitchAsics() = 1;

  const auto switchSettings = generateSwitchSettings(descriptor);

  EXPECT_EQ(*switchSettings.switchType(), cfg::SwitchType::NPU);
  EXPECT_TRUE(*switchSettings.needL2EntryForNeighbor());
  ASSERT_EQ(switchSettings.switchIdToSwitchInfo()->size(), 1);
  const auto& switchInfo = switchSettings.switchIdToSwitchInfo()->at(0);
  EXPECT_EQ(*switchInfo.switchType(), cfg::SwitchType::NPU);
  EXPECT_EQ(*switchInfo.asicType(), cfg::AsicType::ASIC_TYPE_TOMAHAWK5);
  EXPECT_EQ(*switchInfo.switchIndex(), 0);
  EXPECT_EQ(
      *switchInfo.portIdRange()->minimum(),
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN());
  EXPECT_EQ(
      *switchInfo.portIdRange()->maximum(),
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX());
}

TEST(AgentConfigGenTest, GeneratesDefaultAclTableGroup) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(fbossRoot, "test_vendor");

  const auto switchConfig =
      generateSwitchConfigFromArtifacts(fbossRoot, kPlatform);

  cfg::AclTable table;
  table.name() = cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE();
  table.priority() = 0;
  table.aclEntries() = {};
  table.actionTypes() = {};
  table.qualifiers() = {};
  table.udfGroups() = {};
  cfg::AclTableGroup group;
  group.name() = "acl-table-group-ingress";
  group.aclTables() = {table};
  group.stage() = cfg::AclStage::INGRESS;
  const std::vector<cfg::AclTableGroup> expected{group};

  EXPECT_EQ(*switchConfig.aclTableGroups(), expected);
  EXPECT_FALSE(switchConfig.aclTableGroup());
}

TEST(AgentConfigGenTest, ResolvesVariantDescriptorAndRejectsMultiAsicPlatform) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(fbossRoot, "test_vendor");
  const auto generatedMappingDirectory = fbossRoot / "lib" /
      "platform_mapping_v2" / "generated_platform_mappings" / "test_vendor";
  fs::remove(
      generatedMappingDirectory / kPlatform / "platform_descriptor.json");
  const auto variantDescriptor = generatedMappingDirectory /
      "test_platform_variant" / "platform_descriptor.json";
  writePlatformDescriptor(variantDescriptor, 2);

  const auto [descriptorPath, descriptor] =
      findPlatformDescriptorConfigWithDescriptor(fbossRoot, kPlatform);
  EXPECT_EQ(descriptorPath, variantDescriptor);
  EXPECT_EQ(*descriptor.numSwitchAsics(), 2);
  EXPECT_THROW(
      generateSwitchConfigFromArtifacts(fbossRoot, kPlatform), FbossError);
}

TEST(AgentConfigGenTest, GeneratesPlatformConfigFromArtifacts) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(fbossRoot, "test_vendor");

  const auto platformConfig =
      generatePlatformConfigFromArtifacts(fbossRoot, kPlatform, kProfile);

  ASSERT_EQ(
      platformConfig.chip()->getType(), cfg::ChipConfig::Type::asicConfig);
  EXPECT_EQ(
      platformConfig.chip()->get_asicConfig().common()->get_yamlConfig(),
      kAsicYaml);
  EXPECT_EQ(
      *platformConfig.portIdToPortAssignment(),
      makePortAssignments(1, kPortName));
  EXPECT_FALSE(platformConfig.platformSettings());
}

TEST(AgentConfigGenTest, PrefersColocatedPortAssignments) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  const auto platformDirectory = createTestPlatform(fbossRoot, "test_vendor");
  const auto colocatedPath = platformDirectory / "platform_mapping" /
      "generated" / "port_id_to_port_assignment.json";
  writePortAssignments(colocatedPath, 2, "eth1/1/2");

  EXPECT_EQ(
      findPortIdToPortAssignmentConfig(fbossRoot, kPlatform), colocatedPath);
  EXPECT_EQ(
      *generatePlatformConfigFromArtifacts(fbossRoot, kPlatform, kProfile)
           .portIdToPortAssignment(),
      makePortAssignments(2, "eth1/1/2"));
}

TEST(AgentConfigGenTest, GeneratesJsonAsicConfig) {
  folly::test::TemporaryDirectory temporaryDirectory;
  const auto fbossRoot = fs::path(temporaryDirectory.path().string()) / "fboss";
  createTestPlatform(
      fbossRoot, "test_vendor", kPlatform, "JSON_CONFIG", ".json", kAsicJson);

  const auto platformConfig =
      generatePlatformConfigFromArtifacts(fbossRoot, kPlatform, kProfile);
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
      generatePlatformConfigFromArtifacts(fbossRoot, kPlatform, kProfile);
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
      generatePlatformConfigFromArtifacts(fbossRoot, kPlatform, kProfile),
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
  const std::map<std::string, std::string> expectedCommandLineArgs{
      {"enable_acl_table_group", "true"}};
  EXPECT_EQ(*config.defaultCommandLineArgs(), expectedCommandLineArgs);
  EXPECT_EQ(
      *config.sw(), generateSwitchConfigFromArtifacts(fbossRoot, kPlatform));
  EXPECT_EQ(
      *config.platform(),
      generatePlatformConfigFromArtifacts(fbossRoot, kPlatform, kProfile));
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
