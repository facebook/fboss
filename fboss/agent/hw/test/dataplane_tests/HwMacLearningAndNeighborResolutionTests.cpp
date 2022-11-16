/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestLearningUpdateObserver.h"
#include "fboss/agent/hw/test/HwTestMacUtils.h"
#include "fboss/agent/hw/test/HwTestNeighborUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"

namespace facebook::fboss {

namespace {
const AggregatePortID kAggID{1};
const AggregatePortID kAggID2{2};
constexpr int kMinAgeInSecs{1};
const cfg::AclLookupClass kLookupClass{
    cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2};

template <
    cfg::L2LearningMode mode = cfg::L2LearningMode::HARDWARE,
    bool trunk = false>
struct LearningModeAndPortTypesT {
  static constexpr auto kLearningMode = mode;
  static constexpr auto kIsTrunk = trunk;
  static cfg::SwitchConfig initialConfig(cfg::SwitchConfig config) {
    config.switchSettings()->l2LearningMode() = kLearningMode;
    if (kIsTrunk) {
      auto addTrunk = [&config](auto aggId, auto startIdx) {
        std::vector<int> ports;
        auto configPorts = config.vlanPorts();
        for (auto i = startIdx; i < startIdx + 2; ++i) {
          ports.push_back(*(configPorts[i].logicalPort()));
        }
        utility::addAggPort(aggId, ports, &config);
      };
      addTrunk(kAggID, 0);
      addTrunk(kAggID2, 2);
    }
    return config;
  }
};

using SwLearningModeAndTrunk =
    LearningModeAndPortTypesT<cfg::L2LearningMode::SOFTWARE, true>;
using SwLearningModeAndPort =
    LearningModeAndPortTypesT<cfg::L2LearningMode::SOFTWARE, false>;
using HwLearningModeAndTrunk =
    LearningModeAndPortTypesT<cfg::L2LearningMode::HARDWARE, true>;
using HwLearningModeAndPort =
    LearningModeAndPortTypesT<cfg::L2LearningMode::HARDWARE, false>;

using LearningAndPortTypes = ::testing::Types<
    SwLearningModeAndTrunk,
    SwLearningModeAndPort,
    HwLearningModeAndTrunk,
    HwLearningModeAndPort>;

} // namespace

template <typename LearningModeAndPortT>
class HwMacLearningAndNeighborResolutionTest : public HwLinkStateDependentTest {
 public:
  static auto constexpr kLearningMode = LearningModeAndPortT::kLearningMode;
  static auto constexpr kIsTrunk = LearningModeAndPortT::kIsTrunk;

 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto inConfig =
        utility::oneL3IntfNPortConfig(getHwSwitch(), allConfigPorts());
    return LearningModeAndPortT::initialConfig(inConfig);
  }
  PortDescriptor portDescriptor() const {
    return kIsTrunk ? PortDescriptor(kAggID)
                    : PortDescriptor(masterLogicalPortIds()[0]);
  }
  PortDescriptor portDescriptor2() const {
    return kIsTrunk ? PortDescriptor(kAggID2)
                    : PortDescriptor(masterLogicalPortIds()[1]);
  }

  template <typename AddrT>
  AddrT neighborAddr() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return folly::IPAddressV4("1.1.1.2");
    } else {
      return folly::IPAddressV6("1::2");
    }
  }
  void verifyForwarding() {
    enableTrunks();
    for (auto i = 0; i < 5; ++i) {
      verifySentPacket(neighborAddr<folly::IPAddressV4>());
      verifySentPacket(neighborAddr<folly::IPAddressV6>());
    }
  }

  void enableTrunks() {
    applyNewState(utility::enableTrunkPorts(this->getProgrammedState()));
  }

  void learnMac(PortDescriptor port, bool wait = false) {
    HwTestLearningUpdateAutoObserver observer(this->getHwSwitchEnsemble());
    bringUpPorts(physicalPortsFor(port));
    bringDownPorts(physicalPortsExcluding(port));

    // Disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getHwSwitchEnsemble(), 0);
    triggerMacLearning(port);
    if (wait) {
      observer.waitForLearningUpdates(1, 1);
    }
  }

  void learnMacAndProgramNeighbors(
      PortDescriptor port,
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
    learnMac(port);
    programNeighbors(port, lookupClass);
  }
  void updateMacEntry(std::optional<cfg::AclLookupClass> lookupClass) {
    auto newState = getProgrammedState()->clone();
    auto vlan = newState->getVlans()->getVlanIf(kVlanID).get();
    auto macTable = vlan->getMacTable().get();
    macTable = macTable->modify(&vlan, &newState);
    auto macEntry = macTable->getNode(kNeighborMac.toString());
    macTable->updateEntry(
        kNeighborMac, macEntry->getPort(), lookupClass, macEntry->getType());
    applyNewState(newState);
  }

  bool skipTest() const {
    // Neighbor and MAC interaction tests only relevant for
    // HwSwitch that maintain MAC entries for nighbors
    return !getHwSwitch()->needL2EntryForNeighbor();
  }
  void removeNeighbors() {
    removeNeighbor(neighborAddr<folly::IPAddressV4>());
    removeNeighbor(neighborAddr<folly::IPAddressV6>());
  }

  void programNeighbors(
      PortDescriptor port,
      std::optional<cfg::AclLookupClass> lookupClass) {
    programNeighbor(port, neighborAddr<folly::IPAddressV4>(), lookupClass);
    programNeighbor(port, neighborAddr<folly::IPAddressV6>(), lookupClass);
  }

  std::vector<PortID> physicalPortsFor(PortDescriptor port) const {
    if (!port.isAggregatePort()) {
      return {port.phyPortID()};
    } else if (port.aggPortID() == kAggID) {
      return {masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
    } else {
      return {masterLogicalPortIds()[2], masterLogicalPortIds()[3]};
    }
  }
  std::vector<PortID> physicalPortsExcluding(PortDescriptor port) {
    std::vector<PortID> otherPorts;
    auto portDescrPorts = physicalPortsFor(port);
    for (auto configPort : allConfigPorts()) {
      if (std::find(portDescrPorts.begin(), portDescrPorts.end(), configPort) ==
          portDescrPorts.end()) {
        otherPorts.emplace_back(configPort);
      }
    }
    return otherPorts;
  }

 private:
  std::vector<PortID> allConfigPorts() const {
    auto masterLogicalPorts = masterLogicalPortIds();
    return std::vector<PortID>(
        masterLogicalPorts.begin(), masterLogicalPorts.begin() + 4);
  }

  void triggerMacLearning(PortDescriptor port) {
    auto phyPort = *physicalPortsFor(port).begin();
    auto vlanID =
        getProgrammedState()->getPorts()->getPort(phyPort)->getIngressVlan();
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        vlanID,
        kNeighborMac,
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);
    EXPECT_TRUE(getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), phyPort));
  }
  void verifySentPacket(const folly::IPAddress& dstIp) {
    auto firstVlan = getProgrammedState()->getVlans()->cbegin()->second;
    auto intfMac =
        utility::getInterfaceMac(getProgrammedState(), firstVlan->getID());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto srcIp =
        dstIp.isV6() ? folly::IPAddress("1::3") : folly::IPAddress("1.1.1.3");
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        firstVlan->getID(),
        srcMac, // src mac
        intfMac, // dst mac
        srcIp,
        dstIp,
        8000, // l4 src port
        8001 // l4 dst port
    );
    EXPECT_TRUE(
        getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket)));
  }
  template <typename AddrT>
  void programNeighbor(
      PortDescriptor port,
      const AddrT& addr,
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
    auto state = getProgrammedState()->clone();
    auto neighborTable = state->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborEntryTable<AddrT>()
                             ->modify(kVlanID, &state);
    if (neighborTable->getEntryIf(addr)) {
      neighborTable->updateEntry(
          addr,
          kNeighborMac,
          port,
          kIntfID,
          NeighborState::REACHABLE,
          lookupClass);
    } else {
      neighborTable->addEntry(addr, kNeighborMac, port, kIntfID);
      // Update entry to add classid if any
      neighborTable->updateEntry(
          addr,
          kNeighborMac,
          port,
          kIntfID,
          NeighborState::REACHABLE,
          lookupClass);
    }
    applyNewState(state);
  }
  template <typename AddrT>
  void removeNeighbor(const AddrT& ip) {
    auto newState{getProgrammedState()->clone()};
    auto neighborTable = newState->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborEntryTable<AddrT>()
                             ->modify(kVlanID, &newState);

    neighborTable->removeEntry(ip);
    applyNewState(newState);
  }
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
  const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
};

TYPED_TEST_SUITE(HwMacLearningAndNeighborResolutionTest, LearningAndPortTypes);

// Typical scenario where neighbor resolution (ARP, NDP) packet cause MAC
// learning followed by neighbor resolution and programming
TYPED_TEST(
    HwMacLearningAndNeighborResolutionTest,
    learnMacAndProgramNeighbors) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [this]() {
    this->learnMacAndProgramNeighbors(this->portDescriptor());
  };
  auto verify = [this]() { this->verifyForwarding(); };
  this->verifyAcrossWarmBoots(setup, verify);
}
// Learn MAC, program neighbors and now age MAC.
// Packets should still be able to get through
//  - For BCM we don't need L2 entries for switched/routed packets. So MAC aging
//  should have no influence
//  - For SAI we configure static MAC, so it should never age.
TYPED_TEST(
    HwMacLearningAndNeighborResolutionTest,
    learnMacProgramNeighborsAndAgeMac) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [this]() {
    this->learnMacAndProgramNeighbors(this->portDescriptor());
    utility::setMacAgeTimerSeconds(this->getHwSwitchEnsemble(), kMinAgeInSecs);
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs));
  };
  auto verify = [this]() { this->verifyForwarding(); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(
    HwMacLearningAndNeighborResolutionTest,
    learnMacProgramNeighborsAndUpdateMac) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [this]() {
    this->learnMacAndProgramNeighbors(this->portDescriptor());
    // Update neighbor class Id
    this->learnMacAndProgramNeighbors(this->portDescriptor(), kLookupClass);
    // Update MAC class ID
    this->updateMacEntry(kLookupClass);
  };
  auto verify = [this]() { this->verifyForwarding(); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMacLearningAndNeighborResolutionTest, flapMacAndNeighbors) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto program = [this] {
    this->learnMacAndProgramNeighbors(this->portDescriptor());
    // Update neighbor class Id
    this->learnMacAndProgramNeighbors(this->portDescriptor(), kLookupClass);
    // Update MAC class ID
    this->updateMacEntry(kLookupClass);
  };
  auto prune = [this] {
    // Remove neighbors and macs
    this->removeNeighbors();
    // Age out Mac
    utility::setMacAgeTimerSeconds(this->getHwSwitchEnsemble(), kMinAgeInSecs);
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs));
  };
  auto setup = [program, prune]() {
    program();
    prune();
    program();
  };
  auto verify = [this]() { this->verifyForwarding(); };
  this->verifyAcrossWarmBoots(setup, verify);
}

// Typical scenario where neighbor resolution (ARP, NDP) packet cause MAC
// learning followed by neighbor resolution and programming. Post that
// simulate a MAC move
TYPED_TEST(
    HwMacLearningAndNeighborResolutionTest,
    learnMacProgramNeighborsAndMove) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto program = [this](auto port) {
    this->learnMacAndProgramNeighbors(port);
    // Update neighbor class Id
    this->learnMacAndProgramNeighbors(port, kLookupClass);
    // Update MAC class ID
    this->updateMacEntry(kLookupClass);
  };
  auto setup = [this, program]() {
    program(this->portDescriptor());
    // Now move it to port descriptor 2
    program(this->portDescriptor2());
  };
  auto verify = [this]() { this->verifyForwarding(); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(
    HwMacLearningAndNeighborResolutionTest,
    learnMacLinkDownNeighborResolve) {
  if (this->skipTest()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
  }

  auto program = [this](auto port) {
    this->learnMac(port, true /* wait */);
    this->bringDownPorts(this->physicalPortsFor(port));
    this->bringUpPorts(this->physicalPortsFor(port));
  };

  auto setup = [this, program]() {
    auto port = this->portDescriptor();
    program(port);
    this->programNeighbors(port, kLookupClass);
  };

  auto verify = [this]() { this->verifyForwarding(); };

  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
