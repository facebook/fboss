/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmEcmpUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddress.h>
extern "C" {}

using folly::IPAddressV4;
using folly::IPAddressV6;
using std::string;

namespace facebook::fboss {

class BcmEmptyEcmpTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
    return cfg;
  }
  template <typename AddrT>
  void programRoutes(
      const utility::EcmpSetupAnyNPorts<AddrT>& ecmpHelper,
      int ecmpWidth,
      const RoutePrefix<AddrT>& prefix) {
    ecmpHelper.programRoutes(getRouteUpdater(), ecmpWidth, {prefix});
  }
  void runTest(unsigned int ecmpWidth) {
    auto cfg = initialConfig();
    applyNewConfig(cfg);
    auto setup = [=]() {
      for (auto v6Pfx :
           {RoutePrefixV6{IPAddressV6(), 0},
            RoutePrefixV6{IPAddressV6("1::1"), 128}}) {
        programRoutes(
            utility::EcmpSetupAnyNPorts6(getProgrammedState(), kRid),
            ecmpWidth,
            v6Pfx);
      }
      for (auto v4Pfx :
           {RoutePrefixV4{IPAddressV4(), 0},
            RoutePrefixV4{IPAddressV4("1.1.1.1"), 32}}) {
        programRoutes(
            utility::EcmpSetupAnyNPorts4(getProgrammedState(), kRid),
            ecmpWidth,
            v4Pfx);
      }
    };
    auto verify = [=]() {
      auto ecmpCount = utility::getEcmpsInHw(getHwSwitch()).size();
      EXPECT_EQ(2, ecmpCount);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  const RouterID kRid{0};
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> ecmpHelper6_;
  std::unique_ptr<utility::EcmpSetupAnyNPorts4> ecmpHelper4_;
};

TEST_F(BcmEmptyEcmpTest, emptyEcmpRecoveryOnWarmboot) {
  runTest(8);
}

} // namespace facebook::fboss
