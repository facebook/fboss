/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTests.h"

namespace facebook::fboss {

class HwLoadBalancerTestV6EcmpToFlowlet
    : public HwLoadBalancerTest<utility::HwIpV6RoCEEcmpDataPlaneTestUtil> {
  std::unique_ptr<utility::HwIpV6RoCEEcmpDataPlaneTestUtil> getECMPHelper()
      override {
    return std::make_unique<utility::HwIpV6RoCEEcmpDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0));
  }

 private:
  cfg::SwitchConfig initialConfig() const override {
    auto hwSwitch = getHwSwitch();
    auto cfg = utility::onePortPerInterfaceConfig(
        hwSwitch, masterLogicalPortIds(), getAsic()->desiredLoopbackModes());
    return cfg;
  }

  void SetUp() override {
    FLAGS_flowletSwitchingEnable = true;
    HwLoadBalancerTest::SetUp();
  }
};

RUN_HW_LOAD_BALANCER_TEST_FOR_ECMP_TO_DLB(
    HwLoadBalancerTestV6EcmpToFlowlet,
    Ecmp,
    Full,
    FrontPanel)

} // namespace facebook::fboss
