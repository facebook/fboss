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

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwSwitchEnsemble;
namespace utility {

void validateUdfConfig(
    const HwSwitch* hw,
    const std::string& udfGroupName,
    const std::string& udfPackeMatchName);

void validateRemoveUdfGroup(
    const HwSwitch* hw,
    const std::string& udfGroupName,
    int udfGroupId);

void validateRemoveUdfPacketMatcher(
    const HwSwitch* hw,
    const std::string& udfPackeMatchName,
    int udfPacketMatcherId);

int getHwUdfGroupId(const HwSwitch* hw, const std::string& udfGroupName);

int getHwUdfPacketMatcherId(
    const HwSwitch* hw,
    const std::string& udfPackeMatchName);

void validateUdfIdsInQset(
    const HwSwitch* hw,
    const int aclGroupId,
    const bool isSet);

cfg::SwitchConfig addUdfAclRoceOpcodeConfig(cfg::SwitchConfig& cfg);

void validateUdfAclRoceOpcodeConfig(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> curState);

} // namespace utility
} // namespace facebook::fboss
