// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestTrunkUtils.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

namespace facebook::fboss::utility {

void verifyAggregatePortCount(
    const HwSwitchEnsemble* /*ensemble*/,
    uint8_t /*count*/) {}

void verifyAggregatePort(
    const HwSwitchEnsemble* /*ensemble*/,
    AggregatePortID /*aggregatePortID*/) {}

void verifyAggregatePortMemberCount(
    const HwSwitchEnsemble* /*ensemble*/,
    AggregatePortID /*aggregatePortID*/,
    uint8_t /*totalCount*/,
    uint8_t /*currentCount*/) {}

void verifyPktFromAggregatePort(
    const HwSwitchEnsemble* /*ensemble*/,
    AggregatePortID /*aggregatePortID*/) {}

} // namespace facebook::fboss::utility
