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

namespace {
const std::vector<facebook::fboss::LabelForwardingAction::LabelStack> stacks{
    {101, 102, 103},
    {201, 202, 203},
    {301, 302, 303},
    {401, 402, 403},
    {501, 502, 503},
    {601, 602, 603},
    {701, 702, 703},
    {801, 802, 803}};

} // namespace

namespace facebook {
namespace fboss {

class HwLoadBalancerTestV6ToMpls
    : public HwLoadBalancerTest<utility::HwIpV6EcmpDataPlaneTestUtil> {
 public:
  bool skipTest() const override {
    return !getPlatform()->getAsic()->isSupported(HwAsic::Feature::MPLS_ECMP);
  }

 private:
  std::unique_ptr<utility::HwIpV6EcmpDataPlaneTestUtil> getECMPHelper()
      override {
    return std::make_unique<utility::HwIpV6EcmpDataPlaneTestUtil>(
        getHwSwitchEnsemble(), RouterID(0), stacks);
  }
};

RUN_ALL_HW_LOAD_BALANCER_TEST(HwLoadBalancerTestV6ToMpls)

} // namespace fboss
} // namespace facebook
