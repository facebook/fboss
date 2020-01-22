/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/state/Port.h"

namespace facebook::fboss {

void BcmLinkStateDependentTests::SetUp() {
  BcmTest::SetUp();
  if (getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
    // No setup beyond init required for warm boot. We should
    // recover back to the state the switch went down with prior
    // to warm boot
    getHwSwitchEnsemble()->applyInitialConfig(initialConfig());
  }
}

HwLinkStateToggler* BcmLinkStateDependentTests::getLinkToggler() {
  return getHwSwitchEnsemble()->getLinkToggler();
}

void BcmLinkStateDependentTests::bringUpPorts(
    const std::vector<PortID>& ports) {
  getLinkToggler()->bringUpPorts(getProgrammedState(), ports);
}

void BcmLinkStateDependentTests::bringDownPorts(
    const std::vector<PortID>& ports) {
  getLinkToggler()->bringDownPorts(getProgrammedState(), ports);
}

} // namespace facebook::fboss
