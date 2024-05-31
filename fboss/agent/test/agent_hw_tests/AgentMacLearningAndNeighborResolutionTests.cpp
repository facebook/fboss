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

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/L2LearningUpdateObserverUtil.h"
#include "fboss/agent/test/utils/MacTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(intf_nbr_tables);

namespace facebook::fboss {

namespace {
const AggregatePortID kAggID{1};
const AggregatePortID kAggID2{2};
constexpr int kMinAgeInSecs{1};
const cfg::AclLookupClass kLookupClass{
    cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2};

template <
    cfg::L2LearningMode mode = cfg::L2LearningMode::HARDWARE,
    bool trunk = false,
    bool intfNbrTable = false>
struct LearningModeAndPortTypesT {
  static constexpr auto kLearningMode = mode;
  static constexpr auto kIsTrunk = trunk;
  static auto constexpr isIntfNbrTable = intfNbrTable;

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

using SwLearningModeAndTrunkVlanNbrTable =
    LearningModeAndPortTypesT<cfg::L2LearningMode::SOFTWARE, true, false>;
using SwLearningModeAndPortVlanNbrTable =
    LearningModeAndPortTypesT<cfg::L2LearningMode::SOFTWARE, false, false>;
using HwLearningModeAndTrunkVlanNbrTable =
    LearningModeAndPortTypesT<cfg::L2LearningMode::HARDWARE, true, false>;
using HwLearningModeAndPortVlanNbrTable =
    LearningModeAndPortTypesT<cfg::L2LearningMode::HARDWARE, false, false>;

using SwLearningModeAndTrunkIntfNbrTable =
    LearningModeAndPortTypesT<cfg::L2LearningMode::SOFTWARE, true, true>;
using SwLearningModeAndPortIntfNbrTable =
    LearningModeAndPortTypesT<cfg::L2LearningMode::SOFTWARE, false, true>;
using HwLearningModeAndTrunkIntfNbrTable =
    LearningModeAndPortTypesT<cfg::L2LearningMode::HARDWARE, true, true>;
using HwLearningModeAndPortIntfNbrTable =
    LearningModeAndPortTypesT<cfg::L2LearningMode::HARDWARE, false, true>;

using LearningAndPortTypes = ::testing::Types<
    SwLearningModeAndTrunkVlanNbrTable,
    SwLearningModeAndPortVlanNbrTable,
    HwLearningModeAndTrunkVlanNbrTable,
    HwLearningModeAndPortVlanNbrTable,
    SwLearningModeAndTrunkIntfNbrTable,
    SwLearningModeAndPortIntfNbrTable,
    HwLearningModeAndTrunkIntfNbrTable,
    HwLearningModeAndPortIntfNbrTable>;

} // namespace

template <typename LearningModeAndPortT>
class AgentMacLearningAndNeighborResolutionTest : public AgentHwTest {
 public:
  static auto constexpr kLearningMode = LearningModeAndPortT::kLearningMode;
  static auto constexpr kIsTrunk = LearningModeAndPortT::kIsTrunk;
  static auto constexpr isIntfNbrTable = LearningModeAndPortT::isIntfNbrTable;

 protected:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_intf_nbr_tables = isIntfNbrTable;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    std::vector<production_features::ProductionFeature> features = {
        production_features::ProductionFeature::MAC_LEARNING};

    if (isIntfNbrTable) {
      features.push_back(
          production_features::ProductionFeature::INTERFACE_NEIGHBOR_TABLE);
    }
    if (kIsTrunk) {
      features.push_back(production_features::ProductionFeature::LAG);
    }
    return features;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto hwAsics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(hwAsics);
    auto inConfig = utility::oneL3IntfNPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        allConfigPorts(ensemble),
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
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
    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          auto newState = utility::enableTrunkPorts(in);
          return newState;
        },
        "enable trunk ports");
  }

  void learnMac(PortDescriptor port, bool wait = false) {
    L2LearningUpdateObserverUtil l2LearningObserver;
    bringUpPorts(physicalPortsFor(port));
    bringDownPorts(physicalPortsExcluding(port));

    // Disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getAgentEnsemble(), 0);
    l2LearningObserver.startObserving(getAgentEnsemble());
    triggerMacLearning(port);
    if (wait) {
      l2LearningObserver.waitForLearningUpdates(1, 1);
    }
    l2LearningObserver.stopObserving();
    l2LearningObserver.reset();
  }

  void learnMacAndProgramNeighbors(
      PortDescriptor port,
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
    programNeighbors(port, lookupClass);
    learnMac(port);
  }

  void updateMacEntry(std::optional<cfg::AclLookupClass> lookupClass) {
    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          auto newState = in->clone();
          auto vlan = newState->getVlans()->getNodeIf(kVlanID).get();
          auto macTable = vlan->getMacTable().get();
          macTable = macTable->modify(&vlan, &newState);
          auto macEntry = macTable->getNode(kNeighborMac.toString());
          macTable->updateEntry(
              kNeighborMac,
              macEntry->getPort(),
              lookupClass,
              macEntry->getType());
          return newState;
        },
        "update mac entry");
  }
  void verifyMacEntry(std::optional<cfg::AclLookupClass> lookupClass) {
    WITH_RETRIES({
      auto state = getProgrammedState();
      auto vlan = state->getVlans()->getNodeIf(kVlanID);
      auto macTable = vlan->getMacTable();
      auto macEntry = macTable->getNodeIf(kNeighborMac.toString());
      EXPECT_EVENTUALLY_NE(macEntry, nullptr);
      if (macEntry) {
        if (lookupClass) {
          EXPECT_EVENTUALLY_EQ(macEntry->getClassID(), lookupClass.value());
          XLOG(DBG2) << "Lookup class matched";
        } else {
          XLOG(DBG2) << "Lookup class not set; skipping checking";
        }
      }
    });
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

  void verifyNeighborsAndMac(
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt,
      bool verifyMac = true) {
    verifyNeighbor(neighborAddr<folly::IPAddressV4>(), lookupClass);
    verifyNeighbor(neighborAddr<folly::IPAddressV6>(), lookupClass);
    if (verifyMac) {
      verifyMacEntry(lookupClass);
    }
  }

  std::vector<PortID> physicalPortsFor(PortDescriptor& port) const {
    if (!port.isAggregatePort()) {
      return {port.phyPortID()};
    } else if (port.aggPortID() == kAggID) {
      return {masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
    } else {
      return {masterLogicalPortIds()[2], masterLogicalPortIds()[3]};
    }
  }
  std::vector<PortID> physicalPortsExcluding(PortDescriptor& port) {
    std::vector<PortID> otherPorts;
    auto portDescrPorts = physicalPortsFor(port);
    for (auto configPort : allConfigPorts(*getAgentEnsemble())) {
      if (std::find(portDescrPorts.begin(), portDescrPorts.end(), configPort) ==
          portDescrPorts.end()) {
        otherPorts.emplace_back(configPort);
      }
    }
    return otherPorts;
  }

 private:
  std::vector<PortID> allConfigPorts(const AgentEnsemble& ensemble) const {
    auto masterLogicalPorts = ensemble.masterLogicalPortIds();
    return std::vector<PortID>(
        masterLogicalPorts.begin(), masterLogicalPorts.begin() + 4);
  }

  void triggerMacLearning(PortDescriptor port) {
    auto phyPort = *physicalPortsFor(port).begin();
    auto vlanID =
        getProgrammedState()->getPorts()->getNodeIf(phyPort)->getIngressVlan();

    auto txPacket = utility::makeEthTxPacket(
        getSw(),
        vlanID,
        kNeighborMac,
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);
    getAgentEnsemble()->ensureSendPacketOutOfPort(std::move(txPacket), phyPort);
  }
  void verifySentPacket(const folly::IPAddress& dstIp) {
    auto firstVlanID = getProgrammedState()->getVlans()->getFirstVlanID();

    auto intfMac = utility::getInterfaceMac(getProgrammedState(), firstVlanID);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto srcIp =
        dstIp.isV6() ? folly::IPAddress("1::3") : folly::IPAddress("1.1.1.3");
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
        firstVlanID,
        srcMac, // src mac
        intfMac, // dst mac
        srcIp,
        dstIp,
        8000, // l4 src port
        8001 // l4 dst port
    );
    EXPECT_TRUE(
        getAgentEnsemble()->ensureSendPacketSwitched(std::move(txPacket)));
  }
  template <typename AddrT>
  void programNeighbor(
      PortDescriptor port,
      const AddrT& addr,
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
    using NeighborTableT = typename std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          auto state = in->clone();
          NeighborTableT* neighborTable;
          if (isIntfNbrTable) {
            neighborTable = state->getInterfaces()
                                ->getNode(kIntfID)
                                ->template getNeighborEntryTable<AddrT>()
                                ->modify(kIntfID, &state);
          } else {
            neighborTable = state->getVlans()
                                ->getNode(kVlanID)
                                ->template getNeighborEntryTable<AddrT>()
                                ->modify(kVlanID, &state);
          }

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
          return state;
        },
        "program neighbor");
  }

  template <typename AddrT>
  void verifyNeighbor(
      const AddrT& addr,
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
    using NeighborTableT = typename std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;
    WITH_RETRIES({
      NeighborTableT* neighborTable;
      auto state = getProgrammedState();
      if (isIntfNbrTable) {
        neighborTable = state->getInterfaces()
                            ->getNode(kIntfID)
                            ->template getNeighborEntryTable<AddrT>()
                            ->modify(kIntfID, &state);
      } else {
        neighborTable = state->getVlans()
                            ->getNode(kVlanID)
                            ->template getNeighborEntryTable<AddrT>()
                            ->modify(kVlanID, &state);
      }
      auto entry = neighborTable->getEntryIf(addr);
      EXPECT_EVENTUALLY_TRUE(entry);
      if (entry) {
        XLOG(DBG2) << "Neighbor entry matched";
        EXPECT_EVENTUALLY_EQ(entry->getClassID(), lookupClass);
      }
    });
  }

  template <typename AddrT>
  void removeNeighbor(const AddrT& ip) {
    using NeighborTableT = typename std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          auto newState = in->clone();
          NeighborTableT* neighborTable;
          if (isIntfNbrTable) {
            neighborTable = newState->getInterfaces()
                                ->getNode(kIntfID)
                                ->template getNeighborEntryTable<AddrT>()
                                ->modify(kIntfID, &newState);
          } else {
            neighborTable = newState->getVlans()
                                ->getNode(kVlanID)
                                ->template getNeighborEntryTable<AddrT>()
                                ->modify(kVlanID, &newState);
          }

          neighborTable->removeEntry(ip);
          return newState;
        },
        "remove neighbor");
  }
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
  const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
};

TYPED_TEST_SUITE(
    AgentMacLearningAndNeighborResolutionTest,
    LearningAndPortTypes);

// Typical scenario where neighbor resolution (ARP, NDP) packet cause MAC
// learning followed by neighbor resolution and programming
TYPED_TEST(
    AgentMacLearningAndNeighborResolutionTest,
    learnMacAndProgramNeighbors) {
  auto setup = [this]() {
    this->learnMacAndProgramNeighbors(this->portDescriptor());
  };
  auto verify = [this]() {
    auto portDescriptor = this->portDescriptor();
    this->bringDownPorts(this->physicalPortsExcluding(portDescriptor));
    this->verifyNeighborsAndMac(
        std::nullopt, this->kLearningMode == cfg::L2LearningMode::SOFTWARE);
    this->verifyForwarding();
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
// Learn MAC, program neighbors and now age MAC.
// Packets should still be able to get through
//  - For BCM we don't need L2 entries for switched/routed packets. So MAC aging
//  should have no influence
//  - For SAI we configure static MAC, so it should never age.
TYPED_TEST(
    AgentMacLearningAndNeighborResolutionTest,
    learnMacProgramNeighborsAndAgeMac) {
  auto setup = [this]() {
    this->learnMacAndProgramNeighbors(this->portDescriptor());
    utility::setMacAgeTimerSeconds(this->getAgentEnsemble(), kMinAgeInSecs);
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs));
  };
  auto verify = [this]() {
    auto portDescriptor = this->portDescriptor();
    this->bringDownPorts(this->physicalPortsExcluding(portDescriptor));
    this->verifyNeighborsAndMac(std::nullopt, false);
    this->verifyForwarding();
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(
    AgentMacLearningAndNeighborResolutionTest,
    learnMacProgramNeighborsAndUpdateMac) {
  auto setup = [this]() {
    this->learnMacAndProgramNeighbors(this->portDescriptor());
    // Update neighbor class Id
    this->learnMacAndProgramNeighbors(this->portDescriptor(), kLookupClass);
    // Update MAC class ID
    this->updateMacEntry(kLookupClass);
  };
  auto verify = [this]() {
    auto portDescriptor = this->portDescriptor();
    this->bringDownPorts(this->physicalPortsExcluding(portDescriptor));
    this->verifyNeighborsAndMac(
        kLookupClass, this->kLearningMode == cfg::L2LearningMode::SOFTWARE);
    this->verifyForwarding();
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentMacLearningAndNeighborResolutionTest, flapMacAndNeighbors) {
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
    utility::setMacAgeTimerSeconds(this->getAgentEnsemble(), kMinAgeInSecs);
    std::this_thread::sleep_for(std::chrono::seconds(2 * kMinAgeInSecs));
  };
  auto setup = [program, prune]() {
    program();
    prune();
    program();
  };
  auto verify = [this]() {
    auto portDescriptor = this->portDescriptor();
    this->bringDownPorts(this->physicalPortsExcluding(portDescriptor));
    this->verifyNeighborsAndMac(
        kLookupClass, this->kLearningMode == cfg::L2LearningMode::SOFTWARE);
    this->verifyForwarding();
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

// Typical scenario where neighbor resolution (ARP, NDP) packet cause MAC
// learning followed by neighbor resolution and programming. Post that
// simulate a MAC move
TYPED_TEST(
    AgentMacLearningAndNeighborResolutionTest,
    learnMacProgramNeighborsAndMove) {
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
  auto verify = [this]() {
    auto portDescriptor = this->portDescriptor2();
    this->bringDownPorts(this->physicalPortsExcluding(portDescriptor));
    this->verifyNeighborsAndMac(
        kLookupClass, this->kLearningMode == cfg::L2LearningMode::SOFTWARE);
    this->verifyForwarding();
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(
    AgentMacLearningAndNeighborResolutionTest,
    learnMacLinkDownNeighborResolve) {
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

  auto verify = [this]() {
    auto portDescriptor = this->portDescriptor();
    this->bringDownPorts(this->physicalPortsExcluding(portDescriptor));
    this->verifyForwarding();
  };

  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
