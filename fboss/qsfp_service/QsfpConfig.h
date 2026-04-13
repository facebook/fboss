// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"

DECLARE_string(qsfp_config);

namespace facebook::fboss {

inline constexpr auto kPhyHwConfigFileName = "phy_hw_config";

struct QsfpConfig {
  QsfpConfig(cfg::QsfpServiceConfig thriftConfig, std::string rawConfig)
      : thrift(std::move(thriftConfig)), raw(std::move(rawConfig)) {}

  // creators
  static std::unique_ptr<QsfpConfig> fromDefaultFile();
  static std::unique_ptr<QsfpConfig> fromFile(folly::StringPiece path);
  static std::unique_ptr<QsfpConfig> fromRawConfig(const std::string& contents);

  void dumpConfig(folly::StringPiece path) const;

  /*
   * Write the PHY/retimer configuration from qsfp config to a file.
   *
   * If phyConfig is present in QsfpServiceConfig, writes it to
   * FLAGS_qsfp_service_volatile_dir/phy_hw_config.
   * If phyConfig is not present, does nothing.
   * Throws FbossError if write fails.
   */
  void writePhyConfigToFile() const;

  const cfg::QsfpServiceConfig thrift;
  const std::string raw;
};

std::unique_ptr<facebook::fboss::QsfpConfig> createEmptyQsfpConfig();
std::unique_ptr<facebook::fboss::QsfpConfig> createFakeQsfpConfig(
    cfg::QsfpServiceConfig& thriftConfig);
} // namespace facebook::fboss
