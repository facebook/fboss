// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/utils/MacTestUtils.h"
#include "fboss/agent/HwSwitchThriftClientTable.h"
#include "fboss/agent/SwSwitch.h"
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

std::vector<L2EntryThrift> getL2Table(SwSwitch* sw_, bool sdk) {
  std::vector<L2EntryThrift> l2Table;
  if (sw_->isRunModeMultiSwitch()) {
    std::vector<L2EntryThrift> l2TableForSwitch;
    for (const auto& switchId : sw_->getSwitchInfoTable().getSwitchIDs()) {
      sw_->getHwSwitchThriftClientTable()->getHwL2Table(
          switchId, l2TableForSwitch);
      l2Table.insert(
          l2Table.end(), l2TableForSwitch.begin(), l2TableForSwitch.end());
    }
  } else {
    sw_->getMonolithicHwSwitchHandler()->fetchL2Table(&l2Table, sdk);
  }
  return l2Table;
}
} // namespace facebook::fboss::utility
