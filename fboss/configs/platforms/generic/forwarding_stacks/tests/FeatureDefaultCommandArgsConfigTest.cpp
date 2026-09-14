// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>
#include <string>

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

TEST(FeatureDefaultCommandArgsConfigTest, LoadsAllCheckedInConfigs) {
  const auto configRoot = build::getResourcePathStd(
      "fboss/configs/platforms/generic/forwarding_stacks/tests/"
      "feature_default_command_args_configs");
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

} // namespace
} // namespace facebook::fboss::configgen
