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

#include "fboss/agent/hw/test/dataplane_tests/HwEcmpDataPlaneTestUtil.h"

namespace facebook::fboss {

class HwOverflowTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg =
        utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
    utility::addProdFeaturesToConfig(cfg, getHwSwitch());
    return cfg;
  }
  void verifyQoS() {
    // TODO
  }
  void verifyLoadBalacing();
  void verifyCopp() {
    // TODO
  }
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

 protected:
  void setupEcmp();
  void verifyInvariantsPostOverflow() {
    verifyQoS();
    verifyCopp();
    verifyLoadBalacing();
  }
  std::unique_ptr<utility::HwIpV6EcmpDataPlaneTestUtil> ecmpHelper_;
};

} // namespace facebook::fboss
