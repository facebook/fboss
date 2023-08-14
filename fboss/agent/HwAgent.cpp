/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/HwAgent.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

HwAgent::HwAgent(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatformFn,
    int16_t switchIndex)
    : platform_(
          initPlatformFn(std::move(config), hwFeaturesDesired, switchIndex)) {}

HwAgent::HwAgent(std::unique_ptr<Platform> platform)
    : platform_(std::move(platform)) {}

HwInitResult HwAgent::initAgent(
    bool failHwCallsOnWarmboot,
    HwSwitchCallback* callback) {
  auto hwInitResult = getPlatform()->getHwSwitch()->init(
      callback, nullptr, failHwCallsOnWarmboot);
  XLOG(DBG2) << "HwSwitch init done";
  return hwInitResult;
}

} // namespace facebook::fboss
