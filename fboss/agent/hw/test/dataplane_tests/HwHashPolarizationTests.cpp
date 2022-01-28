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

#include "fboss/agent/hw/test/HwHashPolarizationTestUtils.h"

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
  void runTest(
      const cfg::LoadBalancer& firstHash,
      const cfg::LoadBalancer& secondHash,
      bool expectPolarization) {
    std::vector<cfg::LoadBalancer> firstHashes = {firstHash};
    std::vector<cfg::LoadBalancer> secondHashes = {secondHash};
    runTest(firstHashes, secondHashes, expectPolarization);
  }

  void runTest(
      const std::vector<cfg::LoadBalancer>& firstHashes,
      const std::vector<cfg::LoadBalancer>& secondHashes,
      bool expectPolarization) {
    auto setup = [this]() {
      programRoutes<folly::IPAddressV4>();
      programRoutes<folly::IPAddressV6>();
    };
    auto verify = [this, firstHashes, secondHashes, expectPolarization]() {
      runFirstHash(firstHashes);
      auto ethFrames = pktsReceived_.rlock();
      runSecondHash(*ethFrames, secondHashes, expectPolarization);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void runFirstHash(const std::vector<cfg::LoadBalancer>& firstHashes) {
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
      applyNewState(utility::addLoadBalancers(
          getPlatform(), getProgrammedState(), firstHashes));

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
  }

  void runSecondHash(
      const std::vector<utility::EthFrame>& rxPackets,
      const std::vector<cfg::LoadBalancer>& secondHashes,
      bool expectPolarization) {
    XLOG(INFO) << " Num captured packets: " << rxPackets.size();
    auto ecmpPorts = getEcmpPorts();
    auto firstVlan = utility::firstVlanID(getProgrammedState());
    auto mac = utility::getInterfaceMac(getProgrammedState(), firstVlan);
    auto preTestStats = getHwSwitchEnsemble()->getLatestPortStats(ecmpPorts);

    // Set second hash
    applyNewState(utility::addLoadBalancers(
        getPlatform(), getProgrammedState(), secondHashes));
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
    for (auto& ethFrame : rxPackets) {
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
            getStatsDelta(preTestStats, secondHashPortStats), kMaxDeviation),
        !expectPolarization);
  }

 private:
  folly::Synchronized<std::vector<utility::EthFrame>> pktsReceived_;
};

TEST_F(HwHashPolarizationTests, fullXfullHash) {
  auto fullHashCfg = utility::getEcmpFullHashConfig(getPlatform());
  runTest(fullHashCfg, fullHashCfg, true /*expect polarization*/);
}

TEST_F(HwHashPolarizationTests, fullXHalfHash) {
  auto firstHashes = utility::getEcmpFullTrunkFullHashConfig(getPlatform());
  firstHashes[0].seed_ref() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::ECMP, folly::MacAddress("fe:bd:67:0e:09:db"));
  firstHashes[1].seed_ref() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::AGGREGATE_PORT, folly::MacAddress("fe:bd:67:0e:09:db"));

  auto secondHashes = utility::getEcmpHalfTrunkFullHashConfig(getPlatform());
  secondHashes[0].seed_ref() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::ECMP, folly::MacAddress("9a:5d:82:09:3a:d9"));
  secondHashes[1].seed_ref() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::AGGREGATE_PORT, folly::MacAddress("9a:5d:82:09:3a:d9"));

  runTest(firstHashes, secondHashes, false /*expect polarization*/);
}

TEST_F(HwHashPolarizationTests, fullXfullHashWithDifferentSeeds) {
  // Setup 2 identical hashes with only the seed changed
  auto firstHashes = utility::getEcmpFullTrunkFullHashConfig(getPlatform());
  firstHashes[0].seed_ref() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::ECMP, folly::MacAddress("fe:bd:67:0e:09:db"));
  firstHashes[1].seed_ref() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::AGGREGATE_PORT, folly::MacAddress("fe:bd:67:0e:09:db"));
  auto secondHashes = utility::getEcmpFullTrunkFullHashConfig(getPlatform());
  secondHashes[0].seed_ref() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::ECMP, folly::MacAddress("9a:5d:82:09:3a:d9"));
  secondHashes[1].seed_ref() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::AGGREGATE_PORT, folly::MacAddress("9a:5d:82:09:3a:d9"));
  runTest(firstHashes, secondHashes, false /*expect polarization*/);
}

template <HwAsic::AsicType kAsic, bool kSai>
struct HwHashPolarizationTestForAsic : public HwHashPolarizationTests {
  bool isAsic() {
    return getHwSwitchEnsemble()->getPlatform()->getAsic()->getAsicType() ==
        kAsic;
  }
  bool shouldRunTest() {
    // run test only if underlying ASIC and SAI mode matches
    return isAsic() && getHwSwitchEnsemble()->isSai() == kSai;
  }
  void runTest(HwAsic::AsicType type, bool sai) {
    if (!shouldRunTest()) {
      GTEST_SKIP();
      return;
    }
    auto setup = [this]() {
      programRoutes<folly::IPAddressV4>();
      programRoutes<folly::IPAddressV6>();
    };
    auto verify = [=]() {
      auto secondHashes =
          utility::getEcmpFullTrunkFullHashConfig(getPlatform());
      secondHashes[0].seed_ref() = getHwSwitch()->generateDeterministicSeed(
          LoadBalancerID::ECMP, folly::MacAddress("9a:5d:82:09:3a:d9"));
      secondHashes[1].seed_ref() = getHwSwitch()->generateDeterministicSeed(
          LoadBalancerID::AGGREGATE_PORT,
          folly::MacAddress("9a:5d:82:09:3a:d9"));
      auto frames = getFullHashedPackets(type, sai);
      runSecondHash(*frames, secondHashes, false);
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

struct HwHashPolarizationTestForTD2 : HwHashPolarizationTestForAsic<
                                          HwAsic::AsicType::ASIC_TYPE_TRIDENT2,
                                          false> {};

TEST_F(HwHashPolarizationTestForTD2, With_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, false);
}

TEST_F(HwHashPolarizationTestForTD2, With_SAI_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, true);
}

TEST_F(HwHashPolarizationTestForTD2, With_SAI_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForTD2, With_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForTD2, With_TH3) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForTD2, With_TH4) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestForSAITD2
    : HwHashPolarizationTestForAsic<
          HwAsic::AsicType::ASIC_TYPE_TRIDENT2,
          true> {};

TEST_F(HwHashPolarizationTestForSAITD2, With_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, false);
}

TEST_F(HwHashPolarizationTestForSAITD2, With_SAI_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, true);
}

TEST_F(HwHashPolarizationTestForSAITD2, With_SAI_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForSAITD2, With_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForSAITD2, With_TH3) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForSAITD2, With_TH4) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestForTH : HwHashPolarizationTestForAsic<
                                         HwAsic::AsicType::ASIC_TYPE_TOMAHAWK,
                                         false> {};

TEST_F(HwHashPolarizationTestForTH, With_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForTH, With_SAI_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForTH, With_SAI_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, true);
}

TEST_F(HwHashPolarizationTestForTH, With_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, false);
}

TEST_F(HwHashPolarizationTestForTH, With_TH3) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForTH, With_TH4) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestForSAITH
    : HwHashPolarizationTestForAsic<
          HwAsic::AsicType::ASIC_TYPE_TOMAHAWK,
          true> {};

TEST_F(HwHashPolarizationTestForSAITH, With_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForSAITH, With_SAI_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForSAITH, With_SAI_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, true);
}

TEST_F(HwHashPolarizationTestForSAITH, With_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, false);
}

TEST_F(HwHashPolarizationTestForSAITH, With_TH3) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForSAITH, With_TH4) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

struct HwHashPolarizationTestForTH3 : HwHashPolarizationTestForAsic<
                                          HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3,
                                          false> {};

TEST_F(HwHashPolarizationTestForTH3, With_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForTH3, With_SAI_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForTH3, With_SAI_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, true);
}

TEST_F(HwHashPolarizationTestForTH3, With_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, false);
}

TEST_F(HwHashPolarizationTestForTH3, With_TH3) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForTH3, With_TH4) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestForSAITH3
    : HwHashPolarizationTestForAsic<
          HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3,
          true> {};

TEST_F(HwHashPolarizationTestForSAITH3, With_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForSAITH3, With_SAI_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForSAITH3, With_SAI_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, true);
}

TEST_F(HwHashPolarizationTestForSAITH3, With_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, false);
}

TEST_F(HwHashPolarizationTestForSAITH3, With_TH3) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForSAITH3, With_TH4) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestForTH4 : HwHashPolarizationTestForAsic<
                                          HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4,
                                          false> {};

TEST_F(HwHashPolarizationTestForTH4, With_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForTH4, With_SAI_TH) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForTH4, With_SAI_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, true);
}

TEST_F(HwHashPolarizationTestForTH4, With_TD2) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TRIDENT2, false);
}

TEST_F(HwHashPolarizationTestForTH4, With_TH3) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForTH4, With_TH4) {
  runTest(HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}
} // namespace facebook::fboss
