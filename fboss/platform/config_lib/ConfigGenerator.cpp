// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <folly/FileUtil.h>
#include <folly/init/Init.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/data_corral_service/if/gen-cpp2/led_manager_config_types.h"
#include "fboss/platform/fan_service/ConfigValidator.h"
#include "fboss/platform/fan_service/if/gen-cpp2/fan_service_config_types.h"
#include "fboss/platform/fw_util/if/gen-cpp2/fw_util_config_types.h"
#include "fboss/platform/platform_manager/ConfigValidator.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/sensor_service/ConfigValidator.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"
#include "fboss/platform/weutil/if/gen-cpp2/weutil_config_types.h"

// The install_dir flag is a requirement that comes from using a custom buck
// rule which passes in the install location of the header.
DEFINE_string(install_dir, "", "output dir where generated header is placed");

DEFINE_string(json_config_dir, "fboss/platform/configs", "");

namespace fs = std::filesystem;
using namespace facebook::fboss::platform;
using namespace facebook::fboss::platform::data_corral_service;
using namespace facebook::fboss::platform::platform_manager;
using namespace facebook::fboss::platform::sensor_config;
using namespace facebook::fboss::platform::fan_service;
using namespace facebook::fboss::platform::weutil_config;
using namespace facebook::fboss::platform::fw_util_config;
using namespace apache::thrift;

namespace {
constexpr auto kHdrName = "GeneratedConfig.h";
constexpr auto kHdrBegin = R"(#pragma once

#include <string>
#include <unordered_map>

namespace facebook::fboss::platform::configs {
)";
constexpr auto kHdrEnd = R"(
} // facebook::fboss::platform::configs
)";
} // namespace

// Returns configs in a two level map.
// The key in the first level is the service name.
// Each service in turn has a map of platform to config.
std::map<std::string, std::map<std::string, std::string>> getConfigs() {
  std::map<
      std::string /* serviceName */,
      std::map<std::string /* platformName */, std::string /* config */>>
      configs{};

  // Iterate over the per platform directories in FLAGS_json_config_dir.
  for (const auto& perPlatformDir :
       fs::directory_iterator(FLAGS_json_config_dir)) {
    std::string platformName = perPlatformDir.path().filename();
    XLOG(INFO) << fmt::format(
        "Processing Platform {} in {}",
        platformName,
        perPlatformDir.path().c_str());

    std::vector<std::string> serviceNames;
    // Fetch service configs by iterating over each platform directory
    for (const auto& jsonConfig : fs::directory_iterator(perPlatformDir)) {
      XLOG(INFO) << "Processing Config " << jsonConfig.path();
      std::string jsonConfigStr{};
      if (!folly::readFile(jsonConfig.path().c_str(), jsonConfigStr)) {
        XLOG(ERR) << "Could not read file " << jsonConfig.path();
        continue;
      }
      std::string serviceName = jsonConfig.path().stem();
      serviceNames.push_back(serviceName);
      configs[serviceName][platformName] = std::move(jsonConfigStr);
    }
    // Validate fetched service config.
    for (const auto& serviceName : serviceNames) {
      auto& jsonConfigStr = configs[serviceName][platformName];
      try {
        if (serviceName == "sensor_service") {
          auto sensorConfig =
              SimpleJSONSerializer::deserialize<SensorConfig>(jsonConfigStr);
          std::optional<PlatformConfig> platformConfig{std::nullopt};
          // TODO(T207042263) Enable cross-service config validation for Darwin
          // once Darwin onboards PM.
          if (platformName != "darwin") {
            if (!configs["platform_manager"].contains(platformName)) {
              throw std::runtime_error(fmt::format(
                  "Platform Manager config doesn't exist for {}",
                  platformName));
            }
            platformConfig = SimpleJSONSerializer::deserialize<PlatformConfig>(
                configs["platform_manager"][platformName]);
          }
          if (!sensor_service::ConfigValidator().isValid(
                  sensorConfig, platformConfig)) {
            throw std::runtime_error("Invalid sensor_service configuration");
          }
        } else if (serviceName == "fan_service") {
          auto config = SimpleJSONSerializer::deserialize<FanServiceConfig>(
              jsonConfigStr);
          if (!fan_service::ConfigValidator().isValid(config)) {
            throw std::runtime_error("Invalid fan_service configuration");
          }
        } else if (serviceName == "platform_manager") {
          auto config =
              SimpleJSONSerializer::deserialize<PlatformConfig>(jsonConfigStr);
          if (!platform_manager::ConfigValidator().isValid(config)) {
            throw std::runtime_error("Invalid platform_manager configuration");
          }
        } else if (serviceName == "weutil") {
          SimpleJSONSerializer::deserialize<WeutilConfig>(jsonConfigStr);
        } else if (serviceName == "fw_util") {
          SimpleJSONSerializer::deserialize<NewFwUtilConfig>(jsonConfigStr);
        } else if (serviceName == "led_manager") {
          SimpleJSONSerializer::deserialize<LedManagerConfig>(jsonConfigStr);
        } else {
          throw std::runtime_error(
              fmt::format("Unknown service {}", serviceName));
        }
      } catch (std::exception& ex) {
        LOG(FATAL) << fmt::format(
            "Failed to deserialize {} config for {} with error: {}",
            serviceName,
            platformName,
            ex.what());
      }
    }

  } // end per platform iteration

  return configs;
}

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv);
  fs::path hdrPath = fs::path(FLAGS_install_dir) / kHdrName;

  XLOG(INFO) << "Current working directory is: " << fs::current_path();
  XLOG(INFO) << "Json config directory is: " << FLAGS_json_config_dir;
  XLOG(INFO) << "Installation directory is: " << FLAGS_install_dir;
  XLOG(INFO) << "Absolute path of generated file: " << fs::absolute(hdrPath);

  std::ofstream stream(hdrPath);
  auto sg = folly::makeGuard([&] { stream.close(); });

  stream << kHdrBegin;

  for (const auto& [serviceName, configsByPlatform] : getConfigs()) {
    stream << fmt::format(
                  "std::unordered_map<std::string, std::string> {}",
                  serviceName)
           << "{" << std::endl;
    for (const auto& [platformName, config] : configsByPlatform) {
      stream << fmt::format(
                    "{{\"{}\", R\"({})\"}},", platformName.c_str(), config)
             << std::endl;
    }
    stream << "};" << std::endl;
  }

  stream << kHdrEnd;
  return 0;
}
