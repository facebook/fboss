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

#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwLinkStateToggler.h"

namespace facebook::fboss {

class HwLinkStateDependentTest : public HwTest {
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
  /*
   * All link state dependent tests must provide a starting
   * config. This is used in setup to get switch into a state
   * where the initial config is applied and all ports are
   * brought up. This gives us a means to synchronize all
   * port state events which may occurs as part of bringing
   * up the chip. Subsequent ports state changes must be
   * through the bringPort(s){Up,Down} APIs below
   */
  virtual cfg::SwitchConfig initialConfig() const = 0;
  HwLinkStateToggler* getLinkToggler();
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN};
  }
};

} // namespace facebook::fboss
