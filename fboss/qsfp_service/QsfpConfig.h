// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"

DECLARE_string(qsfp_config);

namespace facebook::fboss {

struct QsfpConfig {
  QsfpConfig(cfg::QsfpServiceConfig thriftConfig, std::string rawConfig)
      : thrift(std::move(thriftConfig)), raw(std::move(rawConfig)) {}

  // creators
  static std::unique_ptr<QsfpConfig> fromDefaultFile();
  static std::unique_ptr<QsfpConfig> fromFile(folly::StringPiece path);
  static std::unique_ptr<QsfpConfig> fromRawConfig(const std::string& contents);

  void dumpConfig(folly::StringPiece path) const;
  const cfg::QsfpServiceConfig thrift;
  const std::string raw;
};

std::unique_ptr<facebook::fboss::QsfpConfig> createEmptyQsfpConfig();
std::unique_ptr<facebook::fboss::QsfpConfig> createFakeQsfpConfig(
    cfg::QsfpServiceConfig& thriftConfig);
} // namespace facebook::fboss
