// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwSwitchEnsemble;
namespace utility {

void verifyAggregatePortCount(const HwSwitchEnsemble* ensemble, uint8_t count);

void verifyAggregatePort(
    const HwSwitchEnsemble* ensemble,
    AggregatePortID aggregatePortID);

void verifyAggregatePortMemberCount(
    const HwSwitchEnsemble* ensemble,
    AggregatePortID aggregatePortID,
    uint8_t totalCount,
    uint8_t currentCount);

void verifyPktFromAggregatePort(
    const HwSwitchEnsemble* ensemble,
    AggregatePortID aggregatePortID);

int getTrunkMemberCountInHw(
    const HwSwitch* hw,
    AggregatePortID id,
    int CountInSw);
} // namespace utility
} // namespace facebook::fboss
