// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include <chrono>

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/platform_manager/PlatformExplorer.h"

namespace facebook::fboss::platform::platform_manager {

PlatformExplorer::PlatformExplorer(
    std::chrono::seconds exploreInterval,
    const std::string& configFile,
    const ConfigLib& configLib) {
  std::string pmConfigJson;
  if (configFile.empty()) {
    XLOG(INFO) << "No config file was provided. Inferring from config_lib";
    pmConfigJson = configLib.getPlatformManagerConfig();
  } else {
    XLOG(INFO) << "Using config file: " << configFile;
    if (!folly::readFile(configFile.c_str(), pmConfigJson)) {
      throw std::runtime_error(
          "Can not find sensor config file: " + configFile);
    }
  }

  apache::thrift::SimpleJSONSerializer::deserialize<PlatformConfig>(
      pmConfigJson, platformConfig_);
  XLOG(DBG2) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      platformConfig_);

  scheduler_.addFunction([this]() { explore(); }, exploreInterval);
  scheduler_.start();
}

void PlatformExplorer::explore() {
  XLOG(INFO) << "Exploring the device";
}

} // namespace facebook::fboss::platform::platform_manager
