// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/FileUtil.h>
#include <yaml-cpp/yaml.h>
#include <string>

namespace facebook::fboss {

class PlatformMapping;

class BcmYamlConfig {
 public:
  explicit BcmYamlConfig(std::string baseConfig);

  std::string getConfig();

  const YAML::Node& getGlobalBcmDeviceYamlNode() const {
    return globalNode_;
  }

  const YAML::Node& getTMThresholdYamlNode() const {
    return thresholdNode_;
  }

 private:
  void reloadConfig();

  std::vector<YAML::Node> nodes_;

  // node references
  YAML::Node thresholdNode_;
  YAML::Node globalNode_;

  std::string configStr_;
};

} // namespace facebook::fboss
