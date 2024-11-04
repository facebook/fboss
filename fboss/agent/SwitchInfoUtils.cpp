/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SwitchInfoUtils.h"
#include <atomic>
#include <optional>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {
namespace {

bool withinRange(const cfg::SystemPortRanges& ranges, int64_t id) {
  bool inRange{false};
  for (const auto& sysPortRange : *ranges.systemPortRanges()) {
    if (id >= *sysPortRange.minimum() && id <= *sysPortRange.maximum()) {
      inRange = true;
      break;
    }
  }
  return inRange;
}
} // namespace

const std::map<int64_t, cfg::SwitchInfo> getSwitchInfoFromConfigImpl(
    const cfg::SwitchConfig* config) {
  std::map<int64_t, cfg::SwitchInfo> switchInfoMap;
  if (config && config->switchSettings()->switchIdToSwitchInfo()->size()) {
    for (auto& entry : *config->switchSettings()->switchIdToSwitchInfo()) {
      auto switchInfo = entry.second;
      // TODO - remove this once portIdRange is set everywhere
      if (*switchInfo.portIdRange()->minimum() ==
              *switchInfo.portIdRange()->maximum() &&
          *switchInfo.portIdRange()->maximum() == 0) {
        switchInfo.portIdRange()->minimum() =
            cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN();
        switchInfo.portIdRange()->maximum() =
            cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX();
      }
      switchInfoMap.emplace(entry.first, switchInfo);
    }
  }
  return switchInfoMap;
}

const std::map<int64_t, cfg::SwitchInfo> getSwitchInfoFromConfig() {
  std::unique_ptr<AgentConfig> config;
  try {
    config = AgentConfig::fromDefaultFile();
  } catch (const std::exception&) {
    // expected on devservers where no config file is available
    return std::map<int64_t, cfg::SwitchInfo>();
  }
  auto swConfig = config->thrift.sw();
  return getSwitchInfoFromConfigImpl(&(swConfig.value()));
}

const std::map<int64_t, cfg::SwitchInfo> getSwitchInfoFromConfig(
    const cfg::SwitchConfig* config) {
  if (!config) {
    return getSwitchInfoFromConfig();
  } else {
    return getSwitchInfoFromConfigImpl(config);
  }
}

const std::map<int64_t, cfg::SwitchInfo> getSwitchInfoFromConfig(
    const AgentConfig* config) {
  if (!config) {
    return getSwitchInfoFromConfig();
  }
  auto& swConfig = config->thrift.sw().value();
  return getSwitchInfoFromConfig(&swConfig);
}

const std::optional<cfg::SdkVersion> getSdkVersionFromConfigImpl(
    const cfg::SwitchConfig* config) {
  std::optional<cfg::SdkVersion> sdkVersion = std::nullopt;
  if (config->sdkVersion().has_value()) {
    sdkVersion = *config->sdkVersion();
  }
  return sdkVersion;
}

const std::optional<cfg::SdkVersion> getSdkVersionFromConfig() {
  std::unique_ptr<AgentConfig> config;
  try {
    config = AgentConfig::fromDefaultFile();
  } catch (const std::exception&) {
    return std::nullopt;
  }
  auto swConfig = config->thrift.sw();
  return getSdkVersionFromConfigImpl(&(swConfig.value()));
}

const std::optional<cfg::SdkVersion> getSdkVersionFromConfig(
    const AgentConfig* config) {
  if (!config) {
    return getSdkVersionFromConfig();
  }
  auto& swConfig = config->thrift.sw().value();
  return getSdkVersionFromConfigImpl(&swConfig);
}

bool withinRange(const cfg::SystemPortRanges& ranges, InterfaceID intfId) {
  return withinRange(ranges, static_cast<int64_t>(intfId));
}

bool withinRange(const cfg::SystemPortRanges& ranges, SystemPortID sysPortId) {
  return withinRange(ranges, static_cast<int64_t>(sysPortId));
}

} // namespace facebook::fboss
