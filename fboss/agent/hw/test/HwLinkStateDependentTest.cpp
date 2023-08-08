/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/HwLinkStateToggler.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

void HwLinkStateDependentTest::SetUp() {
  HwTest::SetUp();
  if (getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
    // No setup beyond init required for warm boot. We should
    // recover back to the state the switch went down with prior
    // to warm boot
    getHwSwitchEnsemble()->applyInitialConfig(initialConfig());
    if (isSupported(HwAsic::Feature::LINK_STATE_BASED_ISOLATE)) {
      // For switches that support LINK_STATE_BASED_ISOLATE, force
      // switch to come out of isolate post setup. This is required
      // for data plane to work.
      // TODO: write specialized tests that look at link status and
      // and isolate, unisolate based on threshold.
      auto undrainState = getProgrammedState()->clone();
      auto multiSwitchSettings = undrainState->getSwitchSettings()->clone();
      auto matcher =
          HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(0)}));
      auto settings =
          multiSwitchSettings->getNode(matcher.matcherString())->clone();
      settings->setActualSwitchDrainState(cfg::SwitchDrainState::UNDRAINED);
      multiSwitchSettings->updateNode(matcher.matcherString(), settings);
      undrainState->resetSwitchSettings(multiSwitchSettings);

      applyNewState(undrainState);
    }
  }
}

HwLinkStateToggler* HwLinkStateDependentTest::getLinkToggler() {
  return getHwSwitchEnsemble()->getLinkToggler();
}

void HwLinkStateDependentTest::bringUpPorts(const std::vector<PortID>& ports) {
  getLinkToggler()->bringUpPorts(getProgrammedState(), ports);
}

void HwLinkStateDependentTest::bringDownPorts(
    const std::vector<PortID>& ports) {
  getLinkToggler()->bringDownPorts(getProgrammedState(), ports);
}

} // namespace facebook::fboss
