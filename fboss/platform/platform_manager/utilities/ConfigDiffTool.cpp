// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <iostream>
#include <string>

#include <CLI/CLI.hpp>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/platform_manager/utilities/ConfigDiffer.h"

using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::platform_manager;

int main(int argc, char* argv[]) {
  // Initialize folly with gflags disabled
  folly::InitOptions options;
  options.useGFlags(false);
  folly::Init init(&argc, &argv, options);
  CLI::App app{"Platform Manager Config Diff Tool"};
  app.set_help_all_flag("--help-all", "Print extended help and exit");

  std::string pmunitName;
  std::string configFilePath;
  std::string platformName;

  app.add_option(
      "--pmunit_name",
      pmunitName,
      "PmUnit name to compare versioned configs for (optional, compares all if not specified)");

  // Create a mutually exclusive group where one option is required
  auto* configGroup = app.add_option_group(
      "Config Source (one required)",
      "Specify either platform name or config file path");

  configGroup->add_option(
      "--platform_name", platformName, "Platform name to load config for");

  configGroup->add_option(
      "--config_file", configFilePath, "Path to platform config JSON file");

  configGroup->require_option(1); // Exactly one option must be specified

  app.footer(
      "Examples:\n"
      "  config_diff_tool --platform_name=JANGA800BIC\n"
      "  config_diff_tool --platform_name=JANGA800BIC --pmunit_name=SMB\n"
      "  config_diff_tool --config_file=/path/to/config.json");

  CLI11_PARSE(app, argc, argv);

  try {
    std::string configJson;
    if (!platformName.empty()) {
      configJson = ConfigLib().getPlatformManagerConfig(platformName);
    } else if (!configFilePath.empty()) {
      configJson = ConfigLib(configFilePath).getPlatformManagerConfig();
    } else {
      XLOG(ERR) << "Specify either --config_file or --platform_name";
      return 1;
    }

    PlatformConfig config;
    apache::thrift::SimpleJSONSerializer::deserialize<PlatformConfig>(
        configJson, config);

    ConfigDiffer differ;
    if (!pmunitName.empty()) {
      differ.compareVersionedConfigs(config, pmunitName);
    } else {
      differ.compareVersionedConfigs(config);
    }

    differ.printDifferences();
    return 0;

  } catch (const std::exception& e) {
    XLOGF(ERR, "Error: {}", e.what());
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
