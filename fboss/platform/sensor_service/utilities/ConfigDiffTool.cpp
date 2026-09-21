// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <iostream>
#include <string>

#include <CLI/CLI.hpp>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"
#include "fboss/platform/sensor_service/utilities/ConfigDiffer.h"

using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::sensor_service;

int main(int argc, char* argv[]) {
  folly::InitOptions options;
  options.useGFlags(false);
  folly::Init init(&argc, &argv, options);
  CLI::App app{"Sensor Service Config Diff Tool"};
  app.set_help_all_flag("--help-all", "Print extended help and exit");

  std::string pmunitName;
  std::string configFilePath;
  std::string platformName;

  app.add_option(
      "--pmunit_name",
      pmunitName,
      "PmUnit name to compare versioned sensors for (optional, compares all if not specified)");

  auto configGroup = app.add_option_group(
      "Config Source (one required)",
      "Specify either platform name or config file path");

  configGroup->add_option(
      "--platform_name", platformName, "Platform name to load config for");

  configGroup->add_option(
      "--config_file",
      configFilePath,
      "Path to sensor service config JSON file");

  configGroup->require_option(1);

  app.footer(
      "Examples:\n"
      "  config_diff_tool --platform_name=JANGA800BIC\n"
      "  config_diff_tool --platform_name=JANGA800BIC --pmunit_name=COME\n"
      "  config_diff_tool --config_file=/path/to/sensor_service.json");

  CLI11_PARSE(app, argc, argv);

  try {
    std::string configJson;
    if (!platformName.empty()) {
      configJson = ConfigLib().getSensorServiceConfig(platformName);
    } else if (!configFilePath.empty()) {
      configJson = ConfigLib(configFilePath).getSensorServiceConfig();
    } else {
      XLOG(ERR) << "Specify either --config_file or --platform_name";
      return 1;
    }

    sensor_config::SensorConfig config;
    apache::thrift::SimpleJSONSerializer::deserialize<
        sensor_config::SensorConfig>(configJson, config);

    ConfigDiffer differ;
    if (!pmunitName.empty()) {
      differ.compareVersionedSensors(config, pmunitName);
    } else {
      differ.compareVersionedSensors(config);
    }

    differ.printDifferences();
    return 0;

  } catch (const std::exception& e) {
    XLOGF(ERR, "Error: {}", e.what());
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
