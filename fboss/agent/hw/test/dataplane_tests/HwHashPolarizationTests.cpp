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
#include "fboss/agent/test/TrunkUtils.h"

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/HwHashPolarizationTestUtils.h"

#include <folly/logging/xlog.h>

namespace {
constexpr auto kEcmpWidth = 8;
constexpr auto kMaxDeviation = 25;
const auto kMacAddress01 = folly::MacAddress("fe:bd:67:0e:09:db");
const auto kMacAddress02 = folly::MacAddress("9a:5d:82:09:3a:d9");
} // namespace
namespace facebook::fboss {

class HwHashPolarizationTests : public HwLinkStateDependentTest {
 private:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalInterfacePortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }
  void packetReceived(RxPacket* pkt) noexcept override {
    folly::io::Cursor cursor{pkt->buf()};
    pktsReceived_.wlock()->emplace_back(utility::EthFrame{cursor});
  }
  std::vector<PortID> getEcmpPorts() const {
    auto masterLogicalInterfacePorts = masterLogicalInterfacePortIds();
    return {
        masterLogicalInterfacePorts.begin(),
        masterLogicalInterfacePorts.begin() + kEcmpWidth};
  }
  std::vector<PortDescriptor> getEcmpPortDesc() const {
    auto masterLogicalInterfacePorts = masterLogicalInterfacePortIds();
    return {
        masterLogicalInterfacePorts.begin(),
        masterLogicalInterfacePorts.begin() + kEcmpWidth};
  }

 protected:
  std::map<PortID, HwPortStats> getStatsDelta(
      const std::map<PortID, HwPortStats>& before,
      const std::map<PortID, HwPortStats>& after) const {
    std::map<PortID, HwPortStats> delta;
    for (auto& [portId, beforeStats] : before) {
      // We only care out out bytes for this test
      delta[portId].outBytes_() =
          *after.find(portId)->second.outBytes_() - *beforeStats.outBytes_();
    }
    return delta;
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
  void runTest(
      const cfg::LoadBalancer& firstHash,
      const cfg::LoadBalancer& secondHash,
      bool expectPolarization) {
    std::vector<cfg::LoadBalancer> firstHashes;
    std::vector<cfg::LoadBalancer> secondHashes;
    if (getHwSwitchEnsemble()->isSai()) {
      firstHashes = {
          firstHash,
          utility::getTrunkHalfHashConfig({getPlatform()->getAsic()})};
      secondHashes = {
          secondHash,
          utility::getTrunkHalfHashConfig({getPlatform()->getAsic()})};
    } else {
      firstHashes = {firstHash};
      secondHashes = {secondHash};
    }
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
    auto mac = utility::getFirstInterfaceMac(getProgrammedState());
    auto preTestStats = getHwSwitchEnsemble()->getLatestPortStats(ecmpPorts);
    {
      HwTestPacketTrapEntry trapPkts(
          getHwSwitch(),
          std::set<PortID>{
              ecmpPorts.begin(), ecmpPorts.begin() + kEcmpWidth / 2});
      // Set first hash
      applyNewState(utility::addLoadBalancers(
          getHwSwitchEnsemble(),
          getProgrammedState(),
          firstHashes,
          scopeResolver()));

      for (auto isV6 : {true, false}) {
        utility::pumpTraffic(
            isV6,
            utility::getAllocatePktFn(getHwSwitchEnsemble()),
            utility::getSendPktFunc(getHwSwitchEnsemble()),
            mac,
            firstVlan,
            masterLogicalInterfacePortIds()[kEcmpWidth]);
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
    XLOG(DBG2) << " Num captured packets: " << rxPackets.size();
    auto ecmpPorts = getEcmpPorts();
    auto firstVlan = utility::firstVlanID(getProgrammedState());
    auto mac = utility::getFirstInterfaceMac(getProgrammedState());
    auto preTestStats = getHwSwitchEnsemble()->getLatestPortStats(ecmpPorts);

    // Set second hash
    applyNewState(utility::addLoadBalancers(
        getHwSwitchEnsemble(),
        getProgrammedState(),
        secondHashes,
        scopeResolver()));
    auto makeTxPacket = [=, this](
                            folly::MacAddress srcMac, const auto& ipPayload) {
      return utility::makeUDPTxPacket(
          getHwSwitch(),
          firstVlan,
          srcMac,
          mac,
          ipPayload.header().srcAddr,
          ipPayload.header().dstAddr,
          ipPayload.udpPayload()->header().srcPort,
          ipPayload.udpPayload()->header().dstPort);
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
          std::move(pkt), masterLogicalInterfacePortIds()[kEcmpWidth]);
    }
    auto secondHashPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(getEcmpPorts());
    EXPECT_EQ(
        utility::isLoadBalanced(
            getStatsDelta(preTestStats, secondHashPortStats), kMaxDeviation),
        !expectPolarization);
  }

  folly::Synchronized<std::vector<utility::EthFrame>> pktsReceived_;
};

TEST_F(HwHashPolarizationTests, fullXfullHash) {
  auto fullHashCfg = utility::getEcmpFullHashConfig({getPlatform()->getAsic()});
  runTest(fullHashCfg, fullHashCfg, true /*expect polarization*/);
}

TEST_F(HwHashPolarizationTests, fullXHalfHash) {
  auto firstHashes =
      utility::getEcmpFullTrunkHalfHashConfig({getPlatform()->getAsic()});
  firstHashes[0].seed() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::ECMP, kMacAddress01);
  firstHashes[1].seed() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::AGGREGATE_PORT, kMacAddress01);

  auto secondHashes =
      utility::getEcmpFullTrunkHalfHashConfig({getPlatform()->getAsic()});
  secondHashes[0].seed() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::ECMP, kMacAddress02);
  secondHashes[1].seed() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::AGGREGATE_PORT, kMacAddress02);

  runTest(firstHashes, secondHashes, false /*expect polarization*/);
}

TEST_F(HwHashPolarizationTests, fullXfullHashWithDifferentSeeds) {
  // Setup 2 identical hashes with only the seed changed
  auto firstHashes =
      utility::getEcmpFullTrunkHalfHashConfig({getPlatform()->getAsic()});
  firstHashes[0].seed() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::ECMP, kMacAddress01);
  firstHashes[1].seed() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::AGGREGATE_PORT, kMacAddress01);
  auto secondHashes =
      utility::getEcmpFullTrunkHalfHashConfig({getPlatform()->getAsic()});
  secondHashes[0].seed() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::ECMP, kMacAddress02);
  secondHashes[1].seed() = getHwSwitch()->generateDeterministicSeed(
      LoadBalancerID::AGGREGATE_PORT, kMacAddress02);
  runTest(firstHashes, secondHashes, false /*expect polarization*/);
}

template <cfg::AsicType kAsic, bool kSai>
struct HwHashPolarizationTestForAsic : public HwHashPolarizationTests {
  bool isAsic() {
    return getHwSwitchEnsemble()->getPlatform()->getAsic()->getAsicType() ==
        kAsic;
  }
  bool shouldRunTest() {
    // run test only if underlying ASIC and SAI mode matches
    return isAsic() && getHwSwitchEnsemble()->isSai() == kSai;
  }
  void runTest(cfg::AsicType type, bool sai) {
    if (!shouldRunTest()) {
      GTEST_SKIP();
      return;
    }
    auto setup = [this]() {
      programRoutes<folly::IPAddressV4>();
      programRoutes<folly::IPAddressV6>();
    };
    auto verify = [=, this]() {
      auto secondHashes =
          utility::getEcmpFullTrunkHalfHashConfig({getPlatform()->getAsic()});
      secondHashes[0].seed() = getHwSwitch()->generateDeterministicSeed(
          LoadBalancerID::ECMP, kMacAddress02);
      secondHashes[1].seed() = getHwSwitch()->generateDeterministicSeed(
          LoadBalancerID::AGGREGATE_PORT, kMacAddress02);
      auto frames = getFullHashedPackets(type, sai);
      runSecondHash(*frames, secondHashes, false);
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

struct HwHashPolarizationTestForTH
    : HwHashPolarizationTestForAsic<cfg::AsicType::ASIC_TYPE_TOMAHAWK, false> {
};

TEST_F(HwHashPolarizationTestForTH, With_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForTH, With_SAI_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForTH, With_TH3) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForTH, With_TH4) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestForSAITH
    : HwHashPolarizationTestForAsic<cfg::AsicType::ASIC_TYPE_TOMAHAWK, true> {};

TEST_F(HwHashPolarizationTestForSAITH, With_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForSAITH, With_SAI_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForSAITH, With_TH3) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForSAITH, With_TH4) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

struct HwHashPolarizationTestForTH3
    : HwHashPolarizationTestForAsic<cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false> {
};

TEST_F(HwHashPolarizationTestForTH3, With_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForTH3, With_SAI_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForTH3, With_TH3) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForTH3, With_TH4) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestForSAITH3
    : HwHashPolarizationTestForAsic<cfg::AsicType::ASIC_TYPE_TOMAHAWK3, true> {
};

TEST_F(HwHashPolarizationTestForSAITH3, With_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForSAITH3, With_SAI_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForSAITH3, With_TH3) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForSAITH3, With_TH4) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestForTH4
    : HwHashPolarizationTestForAsic<cfg::AsicType::ASIC_TYPE_TOMAHAWK4, false> {
};

TEST_F(HwHashPolarizationTestForTH4, With_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestForTH4, With_SAI_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestForTH4, With_TH3) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestForTH4, With_TH4) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestSAITH4
    : HwHashPolarizationTestForAsic<cfg::AsicType::ASIC_TYPE_TOMAHAWK4, true> {
};

TEST_F(HwHashPolarizationTestSAITH4, With_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestSAITH4, With_SAI_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestSAITH4, With_TH3) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestSAITH4, With_TH4) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

struct HwHashPolarizationTestSAITH5
    : HwHashPolarizationTestForAsic<cfg::AsicType::ASIC_TYPE_TOMAHAWK5, true> {
};

TEST_F(HwHashPolarizationTestSAITH5, With_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, false);
}

TEST_F(HwHashPolarizationTestSAITH5, With_SAI_TH) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK, true);
}

TEST_F(HwHashPolarizationTestSAITH5, With_TH3) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK3, false);
}

TEST_F(HwHashPolarizationTestSAITH5, With_TH4) {
  runTest(cfg::AsicType::ASIC_TYPE_TOMAHAWK4, false);
}

template <int kNumAggregatePorts, int kAggregatePortWidth>
class HwHashTrunkPolarizationTests : public HwHashPolarizationTests {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalInterfacePortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }

  void addAggregatePorts(cfg::SwitchConfig* cfg) {
    AggregatePortID curAggId{1};
    for (auto i = 0; i < kNumAggregatePorts; ++i) {
      std::vector<int32_t> members(kAggregatePortWidth);
      for (auto j = 0; j < kAggregatePortWidth; ++j) {
        members[j] =
            masterLogicalInterfacePortIds()[i * kAggregatePortWidth + j];
      }
      utility::addAggPort(curAggId++, members, cfg);
    }
  }

  flat_set<PortDescriptor> getAggregatePorts() const {
    flat_set<PortDescriptor> aggregatePorts;
    for (auto i = 0; i < kNumAggregatePorts; ++i) {
      aggregatePorts.insert(PortDescriptor(AggregatePortID(i + 1)));
    }
    return aggregatePorts;
  }

  void configureAggregatePorts() {
    auto cfg = initialConfig();
    addAggregatePorts(&cfg);
    auto state = applyNewConfig(cfg);
    applyNewState(utility::enableTrunkPorts(state));
  }

  template <typename AddrT>
  void programRoutesForAggregatePorts() {
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper{getProgrammedState()};
    applyNewState(
        ecmpHelper.resolveNextHops(getProgrammedState(), getAggregatePorts()));
    ecmpHelper.programRoutes(getRouteUpdater(), getAggregatePorts());
  }

  void configureAggPortsAndRoutes() {
    configureAggregatePorts();
    programRoutesForAggregatePorts<folly::IPAddressV4>();
    programRoutesForAggregatePorts<folly::IPAddressV6>();
  }

  std::vector<PortID> getEgressPorts(
      int numAggregatePorts = kNumAggregatePorts) const {
    auto masterLogicalPorts = masterLogicalInterfacePortIds();
    std::vector<PortID> egressPorts{};
    for (auto i = 0; i < numAggregatePorts; ++i) {
      for (auto j = 0; j < kAggregatePortWidth; ++j) {
        auto port =
            masterLogicalInterfacePortIds()[i * kAggregatePortWidth + j];
        egressPorts.push_back(port);
      }
    }
    return egressPorts;
  }

  void runTrunkTest(
      const std::vector<cfg::LoadBalancer>& firstHashes,
      const std::vector<cfg::LoadBalancer>& secondHashes) {
    auto setup = [this]() { configureAggPortsAndRoutes(); };
    auto verify = [this, firstHashes, secondHashes]() {
      runFirstHashTrunkTest(firstHashes);
      auto ethFrames = pktsReceived_.rlock();
      runSecondHashTrunkTest(*ethFrames, secondHashes, false);
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void runFirstHashTrunkTest(const std::vector<cfg::LoadBalancer>& hashes) {
    auto ports = getEgressPorts(kNumAggregatePorts / 2);
    auto firstVlan = utility::firstVlanID(getProgrammedState());
    auto mac = utility::getFirstInterfaceMac(getProgrammedState());
    auto preTestStats =
        getHwSwitchEnsemble()->getLatestPortStats(getEgressPorts());
    {
      HwTestPacketTrapEntry trapPkts(
          getHwSwitch(), std::set<PortID>{ports.begin(), ports.end()});
      // Set first hash
      applyNewState(utility::addLoadBalancers(
          getHwSwitchEnsemble(),
          getProgrammedState(),
          hashes,
          scopeResolver()));

      auto logicalPorts = masterLogicalInterfacePortIds();
      auto portIter = logicalPorts.end() - 1;

      for (auto isV6 : {true, false}) {
        utility::pumpTraffic(
            isV6,
            utility::getAllocatePktFn(getHwSwitchEnsemble()),
            utility::getSendPktFunc(getHwSwitchEnsemble()),
            mac,
            firstVlan,
            *portIter);
      }
    } // stop capture

    auto firstHashPortStats =
        getHwSwitchEnsemble()->getLatestPortStats(getEgressPorts());
    EXPECT_TRUE(utility::isLoadBalanced(
        getStatsDelta(preTestStats, firstHashPortStats), kMaxDeviation));
  }

  void runSecondHashTrunkTest(
      const std::vector<utility::EthFrame>& rxPackets,
      const std::vector<cfg::LoadBalancer>& secondHashes,
      bool expectPolarization) {
    XLOG(DBG2) << " Num captured packets: " << rxPackets.size();
    auto ports = getEgressPorts();
    auto firstVlan = utility::firstVlanID(getProgrammedState());
    auto mac = utility::getFirstInterfaceMac(getProgrammedState());
    auto preTestStats = getHwSwitchEnsemble()->getLatestPortStats(ports);

    // Set second hash
    applyNewState(utility::addLoadBalancers(
        getHwSwitchEnsemble(),
        getProgrammedState(),
        secondHashes,
        scopeResolver()));
    auto makeTxPacket = [=, this](
                            folly::MacAddress srcMac, const auto& ipPayload) {
      return utility::makeUDPTxPacket(
          getHwSwitch(),
          firstVlan,
          srcMac,
          mac,
          ipPayload.header().srcAddr,
          ipPayload.header().dstAddr,
          ipPayload.udpPayload()->header().srcPort,
          ipPayload.udpPayload()->header().dstPort);
    };

    auto logicalPorts = masterLogicalInterfacePortIds();
    auto portIter = logicalPorts.end() - 1;

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
      getHwSwitch()->sendPacketOutOfPortSync(std::move(pkt), *portIter);
    }

    auto secondHashPortStats = getHwSwitchEnsemble()->getLatestPortStats(ports);
    EXPECT_EQ(
        utility::isLoadBalanced(
            getStatsDelta(preTestStats, secondHashPortStats), kMaxDeviation),
        !expectPolarization);
  }

 public:
  void runFullHalfxFullHalfTest() {
    auto firstHashes =
        utility::getEcmpFullTrunkHalfHashConfig({getPlatform()->getAsic()});
    firstHashes[0].seed() = getHwSwitch()->generateDeterministicSeed(
        LoadBalancerID::ECMP, kMacAddress01);
    firstHashes[1].seed() = getHwSwitch()->generateDeterministicSeed(
        LoadBalancerID::AGGREGATE_PORT, kMacAddress01);

    auto secondHashes =
        utility::getEcmpFullTrunkHalfHashConfig({getPlatform()->getAsic()});
    secondHashes[0].seed() = getHwSwitch()->generateDeterministicSeed(
        LoadBalancerID::ECMP, kMacAddress02);
    secondHashes[1].seed() = getHwSwitch()->generateDeterministicSeed(
        LoadBalancerID::AGGREGATE_PORT, kMacAddress02);

    runTrunkTest(firstHashes, secondHashes);
  }
};

using HwHashTrunk4x3PolarizationTests = HwHashTrunkPolarizationTests<4, 3>;
using HwHashTrunk4x2PolarizationTests = HwHashTrunkPolarizationTests<4, 2>;
using HwHashTrunk8x3PolarizationTests = HwHashTrunkPolarizationTests<8, 3>;
using HwHashTrunk8x2PolarizationTests = HwHashTrunkPolarizationTests<8, 2>;

TEST_F(HwHashTrunk4x3PolarizationTests, FullHalfxFullHalf) {
  runFullHalfxFullHalfTest();
}

TEST_F(HwHashTrunk4x2PolarizationTests, FullHalfxFullHalf) {
  runFullHalfxFullHalfTest();
}

TEST_F(HwHashTrunk8x3PolarizationTests, FullHalfxFullHalf) {
  runFullHalfxFullHalfTest();
}

TEST_F(HwHashTrunk8x2PolarizationTests, FullHalfxFullHalf) {
  runFullHalfxFullHalfTest();
}

} // namespace facebook::fboss
