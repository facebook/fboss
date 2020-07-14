/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateToggler.h"

namespace facebook::fboss {

class BcmLinkStateDependentTests : public BcmTest {
 protected:
  void SetUp() override;
  void applyInitialConfig();
  void bringUpPort(PortID port) {
    bringUpPorts({port});
  }
  void bringDownPort(PortID port) {
    bringDownPorts({port});
  }
  void bringUpPorts(const std::vector<PortID>& ports);
  void bringDownPorts(const std::vector<PortID>& ports);

 private:
  HwLinkStateToggler* getLinkToggler();
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN};
  }
};

} // namespace facebook::fboss
