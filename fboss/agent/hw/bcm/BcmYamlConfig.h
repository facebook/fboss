// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/FileUtil.h>
#include <yaml-cpp/yaml.h>
#include <string>
#include "fboss/agent/hw/bcm/types.h"

namespace facebook::fboss {

class PlatformMapping;

class BcmYamlConfig {
 public:
  BcmYamlConfig() {}

  void setBaseConfig(const std::string& baseConfig);

  std::string getConfig();

  BcmMmuState getMmuState() const;
  bool is128ByteIpv6Enabled() const;
  bool isAlpmEnabled() const;

  void dumpConfig(const std::string& dumpFile);

 private:
  // Forbidden copy constructor and assignment operator
  BcmYamlConfig(BcmYamlConfig const&) = delete;
  BcmYamlConfig& operator=(BcmYamlConfig const&) = delete;

  // Will regenerate configStr_ based on nodes_
  void reloadConfig();

  /*
   * Get a configuration property.
   *
   * The returned std::optional will point to nullopt data if no value
   * is set for the specified property.
   */
  template <typename ValueT>
  std::optional<ValueT> getConfigValue(
      const YAML::Node& node,
      const std::string& name) const {
    auto valueNode = node[name];
    if (valueNode) {
      return valueNode.as<ValueT>();
    }
    return std::nullopt;
  }

  std::vector<YAML::Node> nodes_;

  // node references
  YAML::Node thresholdNode_;
  YAML::Node globalNode_;

  std::string configStr_;
};

} // namespace facebook::fboss
