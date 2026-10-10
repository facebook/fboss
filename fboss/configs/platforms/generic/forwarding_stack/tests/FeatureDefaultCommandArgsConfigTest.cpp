// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <folly/FileUtil.h>
#include <gtest/gtest.h>
#include <tools/cxx/Resources.h>

#include "fboss/agent/FbossError.h"
#include "fboss/cli/fboss2/commands/config/gen/FeatureDefaultCommandArgs.h"

namespace facebook::fboss::configgen {
namespace {

namespace fs = std::filesystem;

constexpr std::string_view kConfigFileName =
    "feature_default_command_args_config.json";

std::string readFile(const fs::path& path) {
  std::string contents;
  if (!folly::readFile(path.c_str(), contents)) {
    throw std::runtime_error("Unable to read test file " + path.string());
  }
  return contents;
}

fs::path getConfigRoot() {
  return build::getResourcePathStd(
      "fboss/configs/platforms/generic/forwarding_stack/tests/"
      "feature_default_command_args_configs");
}

FeatureDefaultCommandArgsConfig loadCheckedInAgentConfig() {
  return parseFeatureDefaultCommandArgsConfig(
      readFile(getConfigRoot() / "agent" / std::string(kConfigFileName)));
}

TEST(FeatureDefaultCommandArgsConfigTest, LoadsAllCheckedInConfigs) {
  const auto configRoot = getConfigRoot();
  size_t configCount = 0;
  for (const auto& entry : fs::recursive_directory_iterator(configRoot)) {
    if (!entry.is_regular_file() ||
        entry.path().filename() != kConfigFileName) {
      continue;
    }
    ++configCount;
    EXPECT_NO_THROW(
        parseFeatureDefaultCommandArgsConfig(readFile(entry.path())))
        << entry.path();
  }
  EXPECT_GT(configCount, 0);
}

TEST(FeatureDefaultCommandArgsConfigTest, ResolvesCheckedInAgentPolicy) {
  struct Expectation {
    std::string_view description;
    std::string_view profile;
    cfg::AsicType asicType;
    PlatformType platformType;
    std::vector<std::pair<std::string_view, std::string_view>> expectedArgs;
    std::vector<std::string_view> absentArgs;
  };
  const std::vector<Expectation> expectations{
      {
          .description = "TH5 default profile",
          .profile = "default",
          .asicType = cfg::AsicType::ASIC_TYPE_TOMAHAWK5,
          .platformType = PlatformType::PLATFORM_WEDGE800BACT,
          .expectedArgs =
              {
                  {"enable_acl_table_group", "true"},
                  {"enable_replayer", "true"},
                  {"led_controlled_through_led_service", "true"},
                  {"multi_switch", "true"},
                  {"platform_descriptor_config_path",
                   "/opt/fboss/share/platform_descriptors/"},
                  {"sai_configure_six_tap", "true"},
                  {"use_full_dlb_scale", "true"},
              },
          .absentArgs = {"multi_npu_platform_mapping"},
      },
      {
          .description = "hardware-test profile",
          .profile = "hw_test",
          .asicType = cfg::AsicType::ASIC_TYPE_TOMAHAWK5,
          .platformType = PlatformType::PLATFORM_WEDGE800BACT,
          .expectedArgs =
              {{"platform_descriptor_config_path",
                "/tmp/platform_descriptors/"}},
          .absentArgs = {"enable_replayer", "multi_switch"},
      },
      {
          .description = "Chenab hardware-test profile",
          .profile = "hw_test",
          .asicType = cfg::AsicType::ASIC_TYPE_CHENAB,
          .platformType = PlatformType::PLATFORM_MINIPACK3N,
          .expectedArgs = {{"sai_user_defined_trap", "true"}},
      },
      {
          .description = "legacy LED platform default profile",
          .profile = "default",
          .asicType = cfg::AsicType::ASIC_TYPE_TOMAHAWK4,
          .platformType = PlatformType::PLATFORM_WEDGE400C,
          .absentArgs = {"led_controlled_through_led_service"},
      },
  };
  const auto config = loadCheckedInAgentConfig();

  for (const auto& expectation : expectations) {
    SCOPED_TRACE(expectation.description);
    const auto args = resolveFeatureDefaultCommandArgs(
        config,
        expectation.profile,
        expectation.asicType,
        expectation.platformType);
    for (const auto& [name, expectedValue] : expectation.expectedArgs) {
      const auto it = args.find(std::string(name));
      ASSERT_NE(it, args.end()) << name;
      EXPECT_EQ(it->second, expectedValue) << name;
    }
    for (const auto name : expectation.absentArgs) {
      EXPECT_FALSE(args.contains(std::string(name))) << name;
    }
  }
}

TEST(FeatureDefaultCommandArgsConfigTest, RejectsInvalidFieldType) {
  EXPECT_THROW(
      parseFeatureDefaultCommandArgsConfig(
          R"({"profileDefaultArgs":[],"features":{}})"),
      std::exception);
}

TEST(FeatureDefaultCommandArgsConfigTest, RejectsUnknownField) {
  EXPECT_THROW(
      parseFeatureDefaultCommandArgsConfig(
          R"({"profileDefaultArgs":{},"features":{"feature":{"args":{"foo":"true"},"autoEnabledWhen":{"configProfiles":{"included":["hw_test"]}}}}})"),
      FbossError);
}

TEST(FeatureDefaultCommandArgsConfigTest, RejectsInvalidSemantics) {
  EXPECT_THROW(
      parseFeatureDefaultCommandArgsConfig(
          R"({"profileDefaultArgs":{},"features":{"feature":{"args":{"foo":"true"},"autoEnableWhen":{}}}})"),
      FbossError);
}

TEST(FeatureDefaultCommandArgsConfigTest, RejectsUnknownAsicType) {
  EXPECT_THROW(
      parseFeatureDefaultCommandArgsConfig(
          R"({"features":{"feature":{"args":{"foo":"true"},"autoEnableWhen":{"asicTypes":{"included":["TOMAHAWK5"]}}}}})"),
      FbossError);
}

TEST(FeatureDefaultCommandArgsConfigTest, RejectsUnknownPlatformType) {
  EXPECT_THROW(
      parseFeatureDefaultCommandArgsConfig(
          R"({"features":{"feature":{"args":{"foo":"true"},"autoEnableWhen":{"platforms":{"included":["wedge800bact"]}}}}})"),
      FbossError);
}

} // namespace
} // namespace facebook::fboss::configgen
