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

namespace facebook {
namespace fboss {
class SwitchState;

} // namespace fboss
} // namespace facebook

namespace facebook {
namespace fboss {
namespace utility {

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
} // namespace utility
} // namespace fboss
} // namespace facebook
