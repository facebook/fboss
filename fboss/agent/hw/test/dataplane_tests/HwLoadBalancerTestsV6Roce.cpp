/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwLoadBalancerTests.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"

namespace facebook::fboss {

class HwLoadBalancerTestV6RoCE
    : public HwLoadBalancerTest<utility::HwIpV6RoCEEcmpDataPlaneTestUtil> {
  std::unique_ptr<utility::HwIpV6RoCEEcmpDataPlaneTestUtil> getECMPHelper()
      override {
    return std::make_unique<utility::HwIpV6RoCEEcmpDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0));
  }

 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitchEnsemble(), masterLogicalPortIds());
    if (isSupported(HwAsic::Feature::SAI_UDF_HASH)) {
      cfg::UdfConfig udfCfg = utility::addUdfHashConfig();
      cfg.udfConfig() = udfCfg;
    }
    return cfg;
  }
};

class HwLoadBalancerNegativeTestV6RoCE
    : public HwLoadBalancerTest<utility::HwIpV6RoCEEcmpDataPlaneTestUtil> {
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
};

// This test verifies that with UDF configuration, traffic not matching UDF port
// is not load balanced.
class HwLoadBalancerNegativeProtocolMatchTestV6RoCE
    : public HwLoadBalancerTest<
          utility::HwIpV6RoCEEcmpDestPortDataPlaneTestUtil> {
  std::unique_ptr<utility::HwIpV6RoCEEcmpDestPortDataPlaneTestUtil>
  getECMPHelper() override {
    return std::make_unique<utility::HwIpV6RoCEEcmpDestPortDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0));
  }

 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitchEnsemble(), masterLogicalPortIds());
    if (isSupported(HwAsic::Feature::SAI_UDF_HASH)) {
      cfg::UdfConfig udfCfg = utility::addUdfHashConfig();
      cfg.udfConfig() = udfCfg;
    }
    return cfg;
  }
};

RUN_HW_LOAD_BALANCER_TEST_FRONT_PANEL(HwLoadBalancerTestV6RoCE, Ecmp, FullUdf)

RUN_HW_LOAD_BALANCER_NEGATIVE_TEST_FRONT_PANEL(
    HwLoadBalancerNegativeTestV6RoCE,
    Ecmp,
    Full)

RUN_HW_LOAD_BALANCER_NEGATIVE_TEST_FRONT_PANEL(
    HwLoadBalancerNegativeProtocolMatchTestV6RoCE,
    Ecmp,
    FullUdf)

} // namespace facebook::fboss
