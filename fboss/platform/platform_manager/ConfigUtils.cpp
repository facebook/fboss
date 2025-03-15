// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ConfigUtils.h"

#include <cctype>

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/platform_manager/ConfigValidator.h"

namespace facebook::fboss::platform::platform_manager {

// Verify that the platform name from the config and dmidecode match.  This
// is necessary to prevent an incorrect config from being used on any platform.
void verifyPlatformNameMatches(
    const std::string& platformNameInConfig,
    const std::string& platformNameFromBios) {
  std::string platformNameInConfigUpper(platformNameInConfig);
  std::transform(
      platformNameInConfigUpper.begin(),
      platformNameInConfigUpper.end(),
      platformNameInConfigUpper.begin(),
      ::toupper);

  if (platformNameInConfigUpper == platformNameFromBios) {
    return;
  }

  XLOGF(
      FATAL,
      "Platform name in config does not match the inferred platform name from "
      "bios. Config: {}, Inferred name from BIOS {} ",
      platformNameInConfigUpper,
      platformNameFromBios);
}

PlatformConfig ConfigUtils::getConfig() {
  std::string platformNameFromBios =
      helpers::PlatformNameLib().getPlatformNameFromBios(true);
  std::string configJson =
      ConfigLib().getPlatformManagerConfig(platformNameFromBios);
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

  verifyPlatformNameMatches(*config.platformName(), platformNameFromBios);

  if (!ConfigValidator().isValid(config)) {
    XLOG(ERR) << "Invalid platform config";
    throw std::runtime_error("Invalid platform config");
  }
  return config;
}
} // namespace facebook::fboss::platform::platform_manager
