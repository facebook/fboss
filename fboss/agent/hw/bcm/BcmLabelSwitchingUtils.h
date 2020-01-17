// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <bcm/mpls.h>
}

#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/LabelForwardingEntry.h"

namespace facebook::fboss::utility {

bcm_mpls_switch_action_t getLabelSwitchAction(
    LabelNextHopEntry::Action action,
    LabelForwardingAction::LabelForwardingType type);

bool isValidLabeledNextHopSet(
    uint32_t maxLabelStackDepth,
    const LabelNextHopSet& nexthops);

bool isValidLabeledNextHop(uint32_t maxLabelStackDepth, const NextHop& nexthop);

RouteNextHopEntry::NextHopSet stripLabelForwarding(
    RouteNextHopEntry::NextHopSet nexthops);

} // namespace facebook::fboss::utility
