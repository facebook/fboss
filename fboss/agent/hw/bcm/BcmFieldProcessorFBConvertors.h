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

#include "fboss/agent/gen-cpp2/switch_config_types.h"

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss::utility {

int swPriorityToHwPriority(int swPrio);
int hwPriorityToSwPriority(int hwPriority);

uint32_t cfgRangeFlagsToBcmRangeFlags(uint32_t flags);
uint32_t bcmRangeFlagsTocfgRangeFlags(uint32_t flags);

bcm_field_IpFrag_t cfgIpFragToBcmIpFrag(cfg::IpFragMatch cfgType);
cfg::IpFragMatch bcmIpFragToCfgIpFrag(bcm_field_IpFrag_t bcmFrag);

void cfgIcmpTypeCodeToBcmIcmpCodeMask(
    std::optional<uint8_t> type,
    std::optional<uint8_t> code,
    uint16_t* bcmCode,
    uint16_t* bcmMask);

void cfgIcmpTypeCodeToBcmIcmpCodeMask(
    std::optional<uint8_t> type,
    std::optional<uint8_t> code,
    bcm_l4_port_t* bcmCode,
    bcm_l4_port_t* bcmMask);

void bcmIcmpTypeCodeToCfgIcmpTypeAndCode(
    uint16_t bcmCode,
    uint16_t bcmMask,
    std::optional<uint8_t>* type,
    std::optional<uint8_t>* code);

void cfgDscpToBcmDscp(uint8_t cfgDscp, uint8* bcmData, uint8* bcmMask);

bcm_field_IpType_t cfgIpTypeToBcmIpType(cfg::IpType cfgType);
cfg::IpType bcmIpTypeToCfgIpType(bcm_field_IpType_t bcmIpType);

bcm_field_stat_t cfgCounterTypeToBcmCounterType(cfg::CounterType cfgType);
cfg::CounterType bcmCounterTypeToCfgCounterType(bcm_field_stat_t bcmType);

} // namespace facebook::fboss::utility
