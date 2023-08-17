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
