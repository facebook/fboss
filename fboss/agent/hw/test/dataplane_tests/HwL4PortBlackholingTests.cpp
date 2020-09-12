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
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/IPAddress.h>

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;

namespace facebook::fboss {

class HwL4PortBlackHolingTest : public HwLinkStateDependentTest {
 private:
  int kNumL4Ports() const {
    return pow(2, 16) - 1;
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    return cfg;
  }
  template <typename ECMP_HELPER>
  void setupECMPForwarding(const ECMP_HELPER& ecmpHelper) {
    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), 1), 1);
    applyNewState(newState);
  }
  void pumpTraffic(bool isV6) {
    auto srcIp = IPAddress(isV6 ? "1001::1" : "100.0.0.1");
    auto dstIp = IPAddress(isV6 ? "2001::1" : "200.0.0.1");
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto mac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    enum class Dir { SRC_PORT, DST_PORT };
    for (auto l4Port = 1; l4Port <= kNumL4Ports(); ++l4Port) {
      for (auto dir : {Dir::SRC_PORT, Dir::DST_PORT}) {
        auto pkt = utility::makeUDPTxPacket(
            getHwSwitch(),
            vlanId,
            mac,
            mac,
            srcIp,
            dstIp,
            dir == Dir::SRC_PORT ? l4Port : 1,
            dir == Dir::DST_PORT ? l4Port : 1);
        getHwSwitch()->sendPacketSwitchedSync(std::move(pkt));
      }
    }
  }

 protected:
  void runTest(bool isV6) {
    auto setup = [=]() {
      auto cfg = initialConfig();
      const RouterID kRid{0};
      setupECMPForwarding(
          utility::EcmpSetupAnyNPorts6(getProgrammedState(), kRid));
      setupECMPForwarding(
          utility::EcmpSetupAnyNPorts4(getProgrammedState(), kRid));
    };
    auto verify = [=]() {
      auto originalPortStats = getLatestPortStats(masterLogicalPortIds());
      int numL4Ports = kNumL4Ports();
      PortID portId = masterLogicalPortIds()[0];
      auto expectPackets = [&originalPortStats, numL4Ports, portId](
                               const auto& newPortStats) -> bool {
        auto original = getPortOutPkts(originalPortStats.at(portId));
        auto current = getPortOutPkts(newPortStats.at(portId));
        XLOGF(
            INFO,
            "Checking current port outBytes ({}) - "
            "original port outBytes: ({}) ==  "
            "2 * number of l4 ports ({})",
            current,
            original,
            2 * numL4Ports);
        return current - original == 2 * numL4Ports;
      };
      pumpTraffic(isV6);
      EXPECT_TRUE(getHwSwitchEnsemble()->waitPortStatsCondition(expectPackets));
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwL4PortBlackHolingTest, v6UDP) {
  runTest(true);
}

TEST_F(HwL4PortBlackHolingTest, v4UDP) {
  runTest(false);
}

} // namespace facebook::fboss
