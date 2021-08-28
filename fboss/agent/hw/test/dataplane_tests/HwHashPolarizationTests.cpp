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
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

class HwHashPolarizationTests : public HwLinkStateDependentTest {
 private:
  static constexpr auto kEcmpWidth = 8;
  static constexpr auto kMaxDeviation = 25;
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }
  void packetReceived(RxPacket* pkt) noexcept override {
    folly::io::Cursor cursor{pkt->buf()};
    pktsReceived_.wlock()->emplace_back(utility::EthFrame{cursor});
  }
  template <typename AddrT>
  void programRoutes() {
    auto ecmpPorts = getEcmpPorts();
    boost::container::flat_set<PortDescriptor> ports;
    std::for_each(ecmpPorts.begin(), ecmpPorts.end(), [&ports](auto ecmpPort) {
      ports.insert(PortDescriptor{ecmpPort});
    });
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(getProgrammedState());
    applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), ports));
    ecmpHelper.programRoutes(getRouteUpdater(), ports);
  }
  std::vector<PortID> getEcmpPorts() const {
    auto masterLogicalPorts = masterLogicalPortIds();
    return {
        masterLogicalPorts.begin(), masterLogicalPorts.begin() + kEcmpWidth};
  }
  std::vector<PortDescriptor> getEcmpPortDesc() const {
    auto masterLogicalPorts = masterLogicalPortIds();
    return {
        masterLogicalPorts.begin(), masterLogicalPorts.begin() + kEcmpWidth};
  }
  std::map<PortID, HwPortStats> getStatsDelta(
      const std::map<PortID, HwPortStats>& before,
      const std::map<PortID, HwPortStats>& after) const {
    std::map<PortID, HwPortStats> delta;
    for (auto& [portId, beforeStats] : before) {
      // We only care out out bytes for this test
      delta[portId].outBytes__ref() =
          *after.find(portId)->second.outBytes__ref() -
          *beforeStats.outBytes__ref();
    }
    return delta;
  }

 protected:
  void runTest(
      const cfg::LoadBalancer& firstHash,
      const cfg::LoadBalancer& secondHash,
      bool expectPolarization) {
    auto setup = [this]() {
      programRoutes<folly::IPAddressV4>();
      programRoutes<folly::IPAddressV6>();
    };
    auto verify = [this, firstHash, secondHash, expectPolarization]() {
      auto ecmpPorts = getEcmpPorts();
      auto firstVlan = utility::firstVlanID(getProgrammedState());
      auto mac = utility::getInterfaceMac(getProgrammedState(), firstVlan);
      auto preTestStats = getHwSwitchEnsemble()->getLatestPortStats(ecmpPorts);
      {
        HwTestPacketTrapEntry trapPkts(
            getHwSwitch(),
            std::set<PortID>{
                ecmpPorts.begin(), ecmpPorts.begin() + kEcmpWidth / 2});
        // Set first hash
        applyNewState(utility::setLoadBalancer(
            getPlatform(), getProgrammedState(), firstHash));

        for (auto isV6 : {true, false}) {
          utility::pumpTraffic(
              isV6,
              getHwSwitch(),
              mac,
              firstVlan,
              masterLogicalPortIds()[kEcmpWidth]);
        }
      } // stop capture

      auto firstHashPortStats =
          getHwSwitchEnsemble()->getLatestPortStats(getEcmpPorts());
      EXPECT_TRUE(utility::isLoadBalanced(
          getStatsDelta(preTestStats, firstHashPortStats), kMaxDeviation));
      XLOG(INFO) << " Num captured packets: " << pktsReceived_.rlock()->size();
      // Set second hash
      applyNewState(utility::setLoadBalancer(
          getPlatform(), getProgrammedState(), secondHash));
      auto makeTxPacket = [=](folly::MacAddress srcMac, const auto& ipPayload) {
        return utility::makeUDPTxPacket(
            getHwSwitch(),
            firstVlan,
            srcMac,
            mac,
            ipPayload.header().srcAddr,
            ipPayload.header().dstAddr,
            ipPayload.payload()->header().srcPort,
            ipPayload.payload()->header().dstPort);
      };
      auto ethFrames = pktsReceived_.rlock();
      for (auto& ethFrame : *ethFrames) {
        std::unique_ptr<TxPacket> pkt;
        auto srcMac = ethFrame.header().srcAddr;
        if (ethFrame.header().etherType ==
            static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
          pkt = makeTxPacket(srcMac, *ethFrame.v4PayLoad());
        } else if (
            ethFrame.header().etherType ==
            static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
          pkt = makeTxPacket(srcMac, *ethFrame.v6PayLoad());
        } else {
          XLOG(FATAL) << " Unexpected packet received, etherType: "
                      << static_cast<int>(ethFrame.header().etherType);
        }
        getHwSwitch()->sendPacketOutOfPortSync(
            std::move(pkt), masterLogicalPortIds()[kEcmpWidth]);
      }
      auto secondHashPortStats =
          getHwSwitchEnsemble()->getLatestPortStats(getEcmpPorts());
      EXPECT_EQ(
          utility::isLoadBalanced(
              getStatsDelta(firstHashPortStats, secondHashPortStats),
              kMaxDeviation),
          !expectPolarization);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  folly::Synchronized<std::vector<utility::EthFrame>> pktsReceived_;
};

TEST_F(HwHashPolarizationTests, fullXfullHash) {
  auto fullHashCfg = utility::getEcmpFullHashConfig(getPlatform());
  runTest(fullHashCfg, fullHashCfg, true /*expect polarization*/);
}

TEST_F(HwHashPolarizationTests, fullXHalfHash) {
  runTest(
      utility::getEcmpFullHashConfig(getPlatform()),
      utility::getEcmpHalfHashConfig(getPlatform()),
      false /*expect polarization*/);
}

TEST_F(HwHashPolarizationTests, fullXfullHashWithDifferentSeeds) {
  // Setup 2 identical hashes with only the seed changed
  auto firstHash = utility::getEcmpFullHashConfig(getPlatform());
  firstHash.seed_ref() = LoadBalancer::generateDeterministicSeed(
      LoadBalancerID::ECMP, folly::MacAddress("fe:bd:67:0e:09:db"));
  auto secondHash = utility::getEcmpFullHashConfig(getPlatform());
  secondHash.seed_ref() = LoadBalancer::generateDeterministicSeed(
      LoadBalancerID::ECMP, folly::MacAddress("9a:5d:82:09:3a:d9"));
  runTest(firstHash, secondHash, false /*expect polarization*/);
}

} // namespace facebook::fboss
