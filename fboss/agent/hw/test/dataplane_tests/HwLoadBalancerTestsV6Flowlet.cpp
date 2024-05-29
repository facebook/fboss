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

class HwLoadBalancerTestV6Flowlet
    : public HwLoadBalancerTest<
          utility::HwIpV6RoCEEcmpDataPlaneTestUtil,
          true /* flowletSwitchingEnable */> {
  std::unique_ptr<utility::HwIpV6RoCEEcmpDataPlaneTestUtil> getECMPHelper()
      override {
    return std::make_unique<utility::HwIpV6RoCEEcmpDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0));
  }

 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitchEnsemble(), masterLogicalPortIds());
    return cfg;
  }

  void SetUp() override {
    FLAGS_flowletSwitchingEnable = true;
    HwLoadBalancerTest::SetUp();
  }
};

RUN_HW_LOAD_BALANCER_TEST_FOR_DLB(
    HwLoadBalancerTestV6Flowlet,
    VerifyWBFromFixedToPerPacket,
    cfg::SwitchingMode::FIXED_ASSIGNMENT,
    cfg::SwitchingMode::PER_PACKET_QUALITY,
    5)

RUN_HW_LOAD_BALANCER_TEST_FOR_DLB(
    HwLoadBalancerTestV6Flowlet,
    VerifyWBFromPerPacketToFixed,
    cfg::SwitchingMode::PER_PACKET_QUALITY,
    cfg::SwitchingMode::FIXED_ASSIGNMENT,
    5)

RUN_HW_LOAD_BALANCER_TEST_FOR_DLB(
    HwLoadBalancerTestV6Flowlet,
    VerifyWBFromFixedToFlowlet,
    cfg::SwitchingMode::FIXED_ASSIGNMENT,
    cfg::SwitchingMode::FLOWLET_QUALITY,
    60)

RUN_HW_LOAD_BALANCER_TEST_FOR_DLB(
    HwLoadBalancerTestV6Flowlet,
    VerifyWBFromFlowletToFixed,
    cfg::SwitchingMode::FLOWLET_QUALITY,
    cfg::SwitchingMode::FIXED_ASSIGNMENT,
    60)

RUN_HW_LOAD_BALANCER_TEST_FOR_DLB(
    HwLoadBalancerTestV6Flowlet,
    VerifyWBFromFlowletToPerPacket,
    cfg::SwitchingMode::FLOWLET_QUALITY,
    cfg::SwitchingMode::PER_PACKET_QUALITY,
    60)

RUN_HW_LOAD_BALANCER_TEST_FOR_DLB(
    HwLoadBalancerTestV6Flowlet,
    VerifyWBFromPerPacketToFlowlet,
    cfg::SwitchingMode::PER_PACKET_QUALITY,
    cfg::SwitchingMode::FLOWLET_QUALITY,
    60)

} // namespace facebook::fboss
