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

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"

namespace facebook::fboss {

class HwLoadBalancerTestV4ToMpls
    : public HwLoadBalancerTest<utility::HwIpV4EcmpDataPlaneTestUtil> {
 public:
  bool skipTest() const override {
    return !isSupported(HwAsic::Feature::MPLS_ECMP);
  }

 private:
  std::unique_ptr<utility::HwIpV4EcmpDataPlaneTestUtil> getECMPHelper()
      override {
    return std::make_unique<utility::HwIpV4EcmpDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0), utility::kHwTestLabelStacks());
  }
};

RUN_ALL_HW_LOAD_BALANCER_TEST(HwLoadBalancerTestV4ToMpls)

} // namespace facebook::fboss
