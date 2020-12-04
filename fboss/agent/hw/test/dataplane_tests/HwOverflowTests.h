/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/HwTestProdConfigUtils.h"

namespace facebook::fboss {

class HwOverflowTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg =
        utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
    utility::addProdFeaturesToConfig(cfg, getHwSwitch());
    return cfg;
  }
  void verifyQoS() const {
    // TODO
  }
  void verifyLoadBalacing() const {
    // TODO
  }
  void verifyCopp() const {
    // TODO
  }

 protected:
  void verifyInvariantsPostOverflow() const {
    verifyQoS();
    verifyCopp();
    verifyLoadBalacing();
  }
};

} // namespace facebook::fboss
