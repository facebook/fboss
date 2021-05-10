// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"

namespace facebook::fboss {

struct QsfpConfig {
  QsfpConfig(cfg::QsfpServiceConfig thriftConfig, std::string rawConfig)
      : thrift(std::move(thriftConfig)), raw(std::move(rawConfig)) {}

  // creators
  static std::unique_ptr<QsfpConfig> fromDefaultFile();
  static std::unique_ptr<QsfpConfig> fromFile(folly::StringPiece path);
  static std::unique_ptr<QsfpConfig> fromRawConfig(const std::string& contents);

  const cfg::QsfpServiceConfig thrift;
  const std::string raw;
};

} // namespace facebook::fboss
