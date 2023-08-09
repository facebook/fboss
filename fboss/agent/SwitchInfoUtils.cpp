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

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {
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
      if (switchInfo.switchType() == cfg::SwitchType::VOQ &&
          !switchInfo.systemPortRange()) {
        auto dsfItr =
            config->dsfNodes()->find(static_cast<int64_t>(entry.first));
        if (dsfItr != config->dsfNodes()->end()) {
          auto localNode = dsfItr->second;
          CHECK(localNode.systemPortRange().has_value());
          switchInfo.systemPortRange() = *localNode.systemPortRange();
        }
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
  } catch (const std::exception& e) {
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
} // namespace facebook::fboss
