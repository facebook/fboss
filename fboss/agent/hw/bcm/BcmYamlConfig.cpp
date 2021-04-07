// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/hw/bcm/BcmYamlConfig.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>

namespace {
constexpr auto kHSDKBcmDeviceKey = "bcm_device";
constexpr auto kHSDKDevice0Key = "0";
constexpr auto kHSDKBcmDeviceGlobalKey = "global";
constexpr auto kHSDKDeviceKey = "device";
constexpr auto kHSDKTMTHDConfigKey = "TM_THD_CONFIG";
} // namespace

namespace facebook::fboss {

using apache::thrift::can_throw;

BcmYamlConfig::BcmYamlConfig(std::string baseConfig) : configStr_(baseConfig) {
  nodes_ = YAML::LoadAll(configStr_);
  for (auto yamlNode : nodes_) {
    if (auto node = yamlNode[kHSDKBcmDeviceKey]) {
      if (auto deviceNode = node[kHSDKDevice0Key]) {
        if (auto globalNode = deviceNode[kHSDKBcmDeviceGlobalKey]) {
          globalNode_ = globalNode;
        }
      }
    } else if (auto node = yamlNode[kHSDKDeviceKey]) {
      if (auto deviceNode = node[kHSDKDevice0Key]) {
        if (auto thresholdNode = deviceNode[kHSDKTMTHDConfigKey]) {
          thresholdNode_ = thresholdNode;
        }
      }
    }
  }
}

std::string BcmYamlConfig::getConfig() {
  return configStr_;
}

} // namespace facebook::fboss
