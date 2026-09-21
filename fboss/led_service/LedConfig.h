// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/led_service/if/gen-cpp2/led_service_config_types.h"

DECLARE_string(led_config);

namespace facebook::fboss {

struct LedConfig {
  LedConfig(cfg::LedServiceConfig thriftConfig, std::string rawConfig)
      : thriftConfig_(std::move(thriftConfig)),
        rawConfig_(std::move(rawConfig)) {}

  // creators
  static std::unique_ptr<LedConfig> fromDefaultFile();
  static std::unique_ptr<LedConfig> fromFile(folly::StringPiece path);
  static std::unique_ptr<LedConfig> fromRawConfig(const std::string& contents);

  void dumpConfig(folly::StringPiece path) const;
  const cfg::LedServiceConfig thriftConfig_;
  const std::string rawConfig_;
};

std::unique_ptr<facebook::fboss::LedConfig> createEmptyLedConfig();
std::unique_ptr<facebook::fboss::LedConfig> createFakeLedConfig(
    cfg::LedServiceConfig& thriftConfig);
} // namespace facebook::fboss
