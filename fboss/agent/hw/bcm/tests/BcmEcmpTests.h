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

#include <folly/IPAddress.h>
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"

namespace facebook::fboss::utility {
template <typename IPAddrT>
class EcmpSetupAnyNPorts;
}

namespace facebook::fboss {

class BcmEcmpTest : public BcmLinkStateDependentTests {
 public:
 protected:
  const RouterID kRid{0};
  const static uint8_t numNextHops_;
  std::unique_ptr<utility::EcmpSetupAnyNPorts<folly::IPAddressV6>> ecmpHelper_;
  std::vector<NextHopWeight> swSwitchWeights_ = {ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT,
                                                 ECMP_WEIGHT};
  std::vector<NextHopWeight> hwSwitchWeights_ = {UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT,
                                                 UCMP_DEFAULT_WEIGHT};
  void SetUp() override;
  cfg::SwitchConfig initialConfig() const override;

  void runSimpleTest(
      const std::vector<NextHopWeight>& swWs,
      const std::vector<NextHopWeight>& hwWs,
      // TODO: Fix warm boot for ECMP and enable warmboot for these tests -
      // T29840275
      bool warmboot = false);
  void runVaryOneNextHopFromHundredTest(
      size_t routeNumNextHops,
      NextHopWeight value,
      const std::vector<NextHopWeight>& hwWs);
  void resolveNhops(int numNhops);
  void resolveNhops(const std::vector<PortDescriptor>& portDescs);
  void programRouteWithUnresolvedNhops(size_t numRouteNextHops = 0);
  const BcmEcmpEgress* getEcmpEgress() const;
  const BcmMultiPathNextHop* getBcmMultiPathNextHop() const;
  int getEcmpSizeInHw(int unit, bcm_if_t ecmp, int sizeInSw);
  std::multiset<bcm_if_t>
  getEcmpGroupInHw(int unit, bcm_if_t ecmp, int sizeInSw);
};

} // namespace facebook::fboss
