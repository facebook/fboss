// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <folly/FileUtil.h>
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>

namespace facebook::fboss {

class BcmYamlConfig {
 public:
  BcmYamlConfig() {}

  void setBaseConfig(const std::string& baseConfig);

  void modifyCoreMaps(
      const std::map<phy::DataPlanePhyChip, std::vector<phy::PinConfig>>&
          pinMapping);

  std::string getConfig();

  std::optional<std::string> getMmuState() const;
  bool is128ByteIpv6Enabled() const;
  bool isAlpmEnabled() const;
  bool isPriorityKeyUsedInRxCosqMapping() const;

  void dumpConfig(const std::string& dumpFile);

  static std::string loadFromFile(const std::string& path);

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
  YAML::Node coreMapNode_;
  YAML::Node thresholdNode_;
  YAML::Node globalNode_;

  std::string configStr_;
  // since we hold the YAML nodes and a string, we need a way to know if there's
  // been modifications
  bool dirty_ = false;
};

} // namespace facebook::fboss
