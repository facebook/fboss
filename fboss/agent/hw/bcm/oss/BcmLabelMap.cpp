// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabelMap.h"

namespace facebook {
namespace fboss {
class BcmLabelSwitchAction {};
BcmLabelMap::~BcmLabelMap() {}
BcmLabelMap::BcmLabelMap(BcmSwitch* /*hw*/) {}

void BcmLabelMap::processAddedLabelSwitchAction(
    BcmLabel /*topLabel*/,
    const LabelNextHopEntry& /*nexthops*/) {}

void BcmLabelMap::processRemovedLabelSwitchAction(BcmLabel /*topLabel*/) {}

void BcmLabelMap::processChangedLabelSwitchAction(
    BcmLabel /*topLabel*/,
    const LabelNextHopEntry& /*nexthops*/) {}

} // namespace fboss
} // namespace facebook
