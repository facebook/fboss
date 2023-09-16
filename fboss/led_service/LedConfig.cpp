// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/led_service/LedConfig.h"
#include "fboss/agent/FbossError.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

DEFINE_string(
    led_config,
    "/etc/coop/led.conf",
    "The path to the local JSON configuration file");

namespace facebook::fboss {

std::unique_ptr<LedConfig> LedConfig::fromDefaultFile() {
  try {
    return fromFile(FLAGS_led_config);
  } catch (const FbossError& err) {
    XLOG(WARN) << " Can't get led config from " << FLAGS_led_config << ": "
               << *err.message();
    throw;
  }
}

std::unique_ptr<LedConfig> LedConfig::fromFile(folly::StringPiece path) {
  std::string configStr;
  if (!folly::readFile(path.data(), configStr)) {
    throw FbossError("unable to read ", path);
  }
  XLOG(DBG2) << "Read led config from " << path;
  return fromRawConfig(std::move(configStr));
}

std::unique_ptr<LedConfig> LedConfig::fromRawConfig(
    const std::string& configStr) {
  cfg::LedServiceConfig ledConfig;

  apache::thrift::SimpleJSONSerializer::deserialize<cfg::LedServiceConfig>(
      configStr, ledConfig);

  return std::make_unique<LedConfig>(std::move(ledConfig), configStr);
}

void LedConfig::dumpConfig(folly::StringPiece path) const {
  folly::writeFile(rawConfig_, path.data());
}

std::unique_ptr<facebook::fboss::LedConfig> createEmptyLedConfig() {
  facebook::fboss::cfg::LedServiceConfig ledCfg;
  return createFakeLedConfig(ledCfg);
}

std::unique_ptr<facebook::fboss::LedConfig> createFakeLedConfig(
    facebook::fboss::cfg::LedServiceConfig& ledCfg) {
  return std::make_unique<facebook::fboss::LedConfig>(
      ledCfg,
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(ledCfg));
}
} // namespace facebook::fboss
