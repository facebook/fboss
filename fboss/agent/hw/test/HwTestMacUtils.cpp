// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestMacUtils.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss::utility {

void setMacAgeTimerSeconds(
    facebook::fboss::HwSwitchEnsemble* hwSwitchEnsemble,
    uint32_t seconds) {
  auto newState = hwSwitchEnsemble->getProgrammedState()->clone();
  auto switchSettings =
      getFirstNodeIf(newState->getMultiSwitchSwitchSettings());
  auto newSwitchSettings = switchSettings->modify(&newState);
  newSwitchSettings->setL2AgeTimerSeconds(seconds);
  hwSwitchEnsemble->applyNewState(newState);
}

} // namespace facebook::fboss::utility
