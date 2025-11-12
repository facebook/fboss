/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/packet/PktFactory.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddress.h>
#include "fboss/agent/TxPacket.h"

using folly::IPAddress;
using folly::IPAddressV6;

namespace facebook::fboss {

class AgentL4PortBlackHolingTest : public AgentHwTest {
 private:
  int kNumL4Ports() const {
    return pow(2, 16) - 1;
  }
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING};
  }
  void pumpTraffic(bool isV6) {
    auto srcIp = IPAddress(isV6 ? "1001::1" : "101.0.0.1");
    auto dstIp = IPAddress(isV6 ? "2001::1" : "201.0.0.1");
    auto vlanId = getVlanIDForTx();
    auto mac = utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    enum class Dir { SRC_PORT, DST_PORT };
    for (auto l4Port = 1; l4Port <= kNumL4Ports(); ++l4Port) {
      for (auto dir : {Dir::SRC_PORT, Dir::DST_PORT}) {
        auto pkt = utility::makeUDPTxPacket(
            getSw(),
            vlanId,
            mac,
            mac,
            srcIp,
            dstIp,
            dir == Dir::SRC_PORT ? l4Port : 1,
            dir == Dir::DST_PORT ? l4Port : 1);
        getSw()->sendPacketSwitchedAsync(std::move(pkt));
      }
    }
  }

 protected:
  void runTest(bool isV6) {
    auto setup = [=, this]() {
      const RouterID kRid{0};
      resolveNeighborAndProgramRoutes(
          utility::EcmpSetupAnyNPorts6(
              getProgrammedState(), getSw()->needL2EntryForNeighbor(), kRid),
          1);
      resolveNeighborAndProgramRoutes(
          utility::EcmpSetupAnyNPorts4(
              getProgrammedState(), getSw()->needL2EntryForNeighbor(), kRid),
          1);
    };
    auto verify = [=, this]() {
      int numL4Ports = kNumL4Ports();
      PortID portId;
      if (FLAGS_hyper_port) {
        portId = masterLogicalHyperPortIds()[0];
      } else {
        portId = masterLogicalInterfacePortIds()[0];
      }
      auto originalPortStats = getLatestPortStats(portId);
      auto original = *originalPortStats.outUnicastPkts_();
      pumpTraffic(isV6);

      WITH_RETRIES_N_TIMED(90, std::chrono::milliseconds(1000), {
        auto newPortStats = getLatestPortStats(portId);
        auto current = *newPortStats.outUnicastPkts_();
        XLOGF(
            INFO,
            "Checking current port outPkts ({}) - "
            "original port outPkts: ({}) ==  "
            "2 * number of l4 ports ({})",
            current,
            original,
            2 * numL4Ports);
        EXPECT_EVENTUALLY_EQ(current - original, 2 * numL4Ports);
      });
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(AgentL4PortBlackHolingTest, v6UDP) {
  runTest(true);
}

TEST_F(AgentL4PortBlackHolingTest, v4UDP) {
  runTest(false);
}

} // namespace facebook::fboss
