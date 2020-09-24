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
#include "fboss/agent/hw/test/HwTestMacUtils.h"
#include "fboss/agent/hw/test/HwTestNeighborUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"

namespace facebook::fboss {

namespace {
const AggregatePortID kAggID{1};

template <
    cfg::L2LearningMode mode = cfg::L2LearningMode::HARDWARE,
    bool trunk = false>
struct LearningModeAndPortTypesT {
  static constexpr auto kLearningMode = mode;
  static constexpr auto kIsTrunk = trunk;
  static cfg::SwitchConfig initialConfig(cfg::SwitchConfig config) {
    config.switchSettings_ref()->l2LearningMode_ref() = kLearningMode;
    if (kIsTrunk) {
      std::vector<int> ports;
      ports.push_back(*config.ports_ref()[0].logicalID_ref());
      ports.push_back(*config.ports_ref()[1].logicalID_ref());
      utility::addAggPort(kAggID, ports, &config);
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
    auto inConfig = kIsTrunk
        ? utility::oneL3IntfTwoPortConfig(
              getHwSwitch(),
              masterLogicalPortIds()[0],
              masterLogicalPortIds()[1])
        : utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
    return LearningModeAndPortT::initialConfig(inConfig);
  }
  PortDescriptor portDescriptor() const {
    return kIsTrunk ? PortDescriptor(kAggID)
                    : PortDescriptor(masterLogicalPortIds()[0]);
  }

  template <typename AddrT>
  AddrT neighborAddr() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return folly::IPAddressV4("1.1.1.2");
    } else {
      return folly::IPAddressV6("1::2");
    }
  }
  void programNeighbors(
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
    programNeighbor(neighborAddr<folly::IPAddressV4>(), lookupClass);
    programNeighbor(neighborAddr<folly::IPAddressV6>(), lookupClass);
  }
  void triggerMacLearning() {
    auto txPacket = utility::makeEthTxPacket(
        getHwSwitch(),
        kVlanID,
        kNeighborMac,
        MacAddress::BROADCAST,
        ETHERTYPE::ETHERTYPE_LLDP);

    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(txPacket), PortID(masterLogicalPortIds()[0]));
  }
  void verifyForwarding() {
    for (auto i = 0; i < 5; ++i) {
      verifySentPacket(neighborAddr<folly::IPAddressV4>());
      verifySentPacket(neighborAddr<folly::IPAddressV6>());
    }
  }

  void learnMacAndProgramNeighbors() {
    // Disable aging, so entry stays in L2 table when we verify.
    utility::setMacAgeTimerSeconds(getHwSwitch(), 0);
    triggerMacLearning();
    programNeighbors();
  }

 private:
  void verifySentPacket(const folly::IPAddress& dstIp) {
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), kVlanID);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto srcIp =
        dstIp.isV6() ? folly::IPAddress("1::3") : folly::IPAddress("1.1.1.3");
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        kVlanID,
        srcMac, // src mac
        intfMac, // dst mac
        srcIp,
        dstIp,
        8000, // l4 src port
        8001 // l4 dst port
    );
    getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
  }
  template <typename AddrT>
  void programNeighbor(
      const AddrT addr,
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
    using NTable = typename std::conditional_t<
        std::is_same_v<AddrT, folly::IPAddressV4>,
        ArpTable,
        NdpTable>;
    auto state = getProgrammedState()->clone();
    auto neighborTable = state->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborTable<NTable>()
                             ->modify(kVlanID, &state);
    if (neighborTable->getEntryIf(addr)) {
      neighborTable->updateEntry(
          addr, kNeighborMac, portDescriptor(), kIntfID, lookupClass);
    } else {
      neighborTable->addEntry(addr, kNeighborMac, portDescriptor(), kIntfID);
      // Update entry to add classid if any
      neighborTable->updateEntry(
          addr, kNeighborMac, portDescriptor(), kIntfID, lookupClass);
    }
    applyNewState(state);
  }
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
  const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
  const cfg::AclLookupClass kLookupClass{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2};
};

TYPED_TEST_SUITE(HwMacLearningAndNeighborResolutionTest, LearningAndPortTypes);

// Typical scenario where neighbor resolution (ARP, NDP) packet cause MAC
// learning followed by neighbor resolution and programming
TYPED_TEST(
    HwMacLearningAndNeighborResolutionTest,
    learnMacAndProgramNeighbors) {
  auto setup = [this]() { this->learnMacAndProgramNeighbors(); };
  auto verify = [this]() { this->verifyForwarding(); };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
