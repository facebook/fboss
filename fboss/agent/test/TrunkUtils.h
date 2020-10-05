/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <memory>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;

} // namespace facebook::fboss

namespace facebook::fboss::utility {

cfg::AggregatePortMember makePortMember(int32_t port);
void addAggPort(
    int key,
    const std::vector<int32_t>& ports,
    cfg::SwitchConfig* config);
std::shared_ptr<SwitchState> enableTrunkPorts(
    std::shared_ptr<SwitchState> curState);
std::shared_ptr<SwitchState> setTrunkMinLinkCount(
    std::shared_ptr<SwitchState> curState,
    uint8_t minlinks);
std::shared_ptr<SwitchState> disableTrunkPort(
    std::shared_ptr<SwitchState> curState,
    const AggregatePortID& aggId,
    const facebook::fboss::PortID& portId);

} // namespace facebook::fboss::utility
