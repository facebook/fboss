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

constexpr auto kHSDKL3ALPMState = "l3_alpm_template";
constexpr auto kHSDKIs128ByteIpv6Enabled = "ipv6_lpm_128b_enable";
constexpr auto kHSDKThresholsModeKey = "THRESHOLD_MODE";
} // namespace

namespace facebook::fboss {

using apache::thrift::can_throw;

void BcmYamlConfig::setBaseConfig(const std::string& baseConfig) {
  configStr_ = baseConfig;
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

BcmMmuState BcmYamlConfig::getMmuState() const {
  auto mode =
      getConfigValue<std::string>(thresholdNode_, kHSDKThresholsModeKey);
  if (mode) {
    XLOG(INFO) << "MMU state is " << *mode;
    if (*mode == "LOSSY") {
      return BcmMmuState::MMU_LOSSY;
    } else if (*mode == "LOSSLESS") {
      return BcmMmuState::MMU_LOSSLESS;
    }
  }
  return BcmMmuState::UNKNOWN;
}

bool BcmYamlConfig::is128ByteIpv6Enabled() const {
  auto state = getConfigValue<int>(globalNode_, kHSDKIs128ByteIpv6Enabled);
  if (!state || *state != 1) {
    return false;
  }
  return true;
}

bool BcmYamlConfig::isAlpmEnabled() const {
  auto state = getConfigValue<int>(globalNode_, kHSDKL3ALPMState);
  if (!state || *state == 0) {
    return false;
  }
  // 1: combined mode, 2: parallel mode. But both are alpm enabled
  return true;
}

void BcmYamlConfig::dumpConfig(const std::string& dumpFile) {
  folly::writeFile(getConfig(), dumpFile.data());
}

std::string BcmYamlConfig::getConfig() {
  return configStr_;
}
} // namespace facebook::fboss
