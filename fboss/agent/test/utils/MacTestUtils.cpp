// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/utils/MacTestUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestEnsembleIf.h"

namespace facebook::fboss::utility {

void setMacAgeTimerSeconds(
    facebook::fboss::TestEnsembleIf* ensemble,
    uint32_t seconds) {
  ensemble->applyNewState(
      [&](const std::shared_ptr<SwitchState>& in) {
        auto newState = in->clone();
        auto switchSettings =
            utility::getFirstNodeIf(newState->getSwitchSettings());
        auto newSwitchSettings = switchSettings->modify(&newState);
        newSwitchSettings->setL2AgeTimerSeconds(seconds);
        return newState;
      },
      "set mac age timer");
}

} // namespace facebook::fboss::utility
