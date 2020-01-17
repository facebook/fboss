// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabelMap.h"

#include "fboss/agent/hw/bcm/BcmLabelSwitchAction.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss {

BcmLabelMap::BcmLabelMap(BcmSwitch* hw) : hw_(hw) {}

BcmLabelMap::~BcmLabelMap() {}

void BcmLabelMap::processAddedLabelSwitchAction(
    BcmLabel topLabel,
    const LabelNextHopEntry& nexthops) {
  labelMap_.emplace(
      topLabel,
      std::make_unique<BcmLabelSwitchAction>(hw_, topLabel, nexthops));
}

void BcmLabelMap::processRemovedLabelSwitchAction(BcmLabel topLabel) {
  labelMap_.erase(topLabel);
}

void BcmLabelMap::processChangedLabelSwitchAction(
    BcmLabel topLabel,
    const LabelNextHopEntry& nexthops) {
  processRemovedLabelSwitchAction(topLabel);
  processAddedLabelSwitchAction(topLabel, nexthops);
}

} // namespace facebook::fboss
