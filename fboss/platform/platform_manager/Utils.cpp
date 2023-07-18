// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/Utils.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/platform_manager/PlatformValidator.h"

namespace facebook::fboss::platform::platform_manager {

PlatformConfig Utils::getConfig(const std::string& configFile) {
  std::string configJson;
  if (configFile.empty()) {
    XLOG(INFO) << "No config file was provided. Inferring from config_lib";
    configJson = ConfigLib().getPlatformManagerConfig();
  } else {
    XLOG(INFO) << "Using config file: " << configFile;
    if (!folly::readFile(configFile.c_str(), configJson)) {
      XLOG(ERR) << "Can not find sensor config file: " + configFile;
      throw std::runtime_error(
          "Can not find sensor config file: " + configFile);
    }
  }

  PlatformConfig config;
  try {
    apache::thrift::SimpleJSONSerializer::deserialize<PlatformConfig>(
        configJson, config);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to deserialize platform config: " << e.what();
    throw;
  }
  XLOG(DBG2) << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
      config);

  if (!PlatformValidator().isValid(config)) {
    XLOG(ERR) << "Invalid platform config";
    throw std::runtime_error("Invalid platform config");
  }
  return config;
}

} // namespace facebook::fboss::platform::platform_manager
