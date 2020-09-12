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
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

template <typename AddrT>
class HwQueuePerHostTest : public HwLinkStateDependentTest {
  using NeighborTableT = typename std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addQueuePerHostQueueConfig(&cfg, masterLogicalPortIds()[0]);
      utility::addQueuePerHostAcls(&cfg);
    }
    return cfg;
  }

  void setupECMPForwarding() {
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper{getProgrammedState()};
    auto kEcmpWidthForTest = 1;

    auto newState = ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidthForTest),
        kEcmpWidthForTest);
    applyNewState(newState);
  }

  AddrT kSrcIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("129.0.0.1");
    } else {
      return folly::IPAddressV6("2620:0:1cfe:face:b00c::1");
    }
  }

  const std::map<AddrT, std::pair<folly::MacAddress, cfg::AclLookupClass>>&
  getIpToMacAndClassID() {
    // TODO (skhare) Use ResourceGenerator to create this map, where the number
    // of entries equals kQueuePerhostQueueIds()

    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      static const std::map<
          folly::IPAddressV4,
          std::pair<folly::MacAddress, cfg::AclLookupClass>>
          ipToMacAndClassID = {
              {folly::IPAddressV4("129.0.0.10"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:10"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0)},
              {folly::IPAddressV4("129.0.0.11"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:11"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1)},
              {folly::IPAddressV4("129.0.0.12"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:12"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2)},
              {folly::IPAddressV4("129.0.0.13"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:13"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3)},
              {folly::IPAddressV4("129.0.0.14"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:14"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4)},
          };

      return ipToMacAndClassID;
    } else {
      static const std::map<
          folly::IPAddressV6,
          std::pair<folly::MacAddress, cfg::AclLookupClass>>
          ipToMacAndClassID = {
              {folly::IPAddressV6("2620:0:1cfe:face:b00c::10"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:10"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0)},
              {folly::IPAddressV6("2620:0:1cfe:face:b00c::11"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:10"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1)},
              {folly::IPAddressV6("2620:0:1cfe:face:b00c::12"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:10"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2)},
              {folly::IPAddressV6("2620:0:1cfe:face:b00c::13"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:10"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3)},
              {folly::IPAddressV6("2620:0:1cfe:face:b00c::14"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:10"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4)},
          };
      return ipToMacAndClassID;
    }
  }

  std::shared_ptr<SwitchState> addNeighbors(
      const std::shared_ptr<SwitchState>& inState) {
    auto outState{inState->clone()};

    for (const auto& ipToMacAndClassID : getIpToMacAndClassID()) {
      auto ip = ipToMacAndClassID.first;
      auto neighborTable = outState->getVlans()
                               ->getVlan(kVlanID)
                               ->template getNeighborTable<NeighborTableT>()
                               ->modify(kVlanID, &outState);
      neighborTable->addPendingEntry(ip, kIntfID);
    }

    return outState;
  }

  std::shared_ptr<SwitchState> updateNeighbors(
      const std::shared_ptr<SwitchState>& inState,
      bool setClassIDs) {
    auto outState{inState->clone()};

    for (const auto& ipToMacAndClassID : getIpToMacAndClassID()) {
      auto ip = ipToMacAndClassID.first;
      auto macAndClassID = ipToMacAndClassID.second;
      auto neighborMac = macAndClassID.first;
      auto classID = macAndClassID.second;
      auto neighborTable = outState->getVlans()
                               ->getVlan(kVlanID)
                               ->template getNeighborTable<NeighborTableT>()
                               ->modify(kVlanID, &outState);
      if (setClassIDs) {
        neighborTable->updateEntry(
            ip,
            neighborMac,
            PortDescriptor(masterLogicalPortIds()[0]),
            kIntfID,
            classID);

      } else {
        neighborTable->updateEntry(
            ip,
            neighborMac,
            PortDescriptor(masterLogicalPortIds()[0]),
            kIntfID);
      }
    }

    return outState;
  }

  std::shared_ptr<SwitchState> resolveNeighbors(
      const std::shared_ptr<SwitchState>& inState) {
    return updateNeighbors(inState, false /* setClassIDs */);
  }

  std::shared_ptr<SwitchState> updateClassID(
      const std::shared_ptr<SwitchState>& inState) {
    return updateNeighbors(inState, true /* setClassIDs */);
  }

  void sendUdpPkt(AddrT dstIP) {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    getHwSwitchEnsemble()->ensureSendPacketSwitched(utility::makeUDPTxPacket(
        getHwSwitch(), vlanId, intfMac, intfMac, kSrcIP(), dstIP, 8000, 8001));
  }

  void sendUdpPkts() {
    for (const auto& ipToMacAndClassID : getIpToMacAndClassID()) {
      auto dstIP = ipToMacAndClassID.first;
      sendUdpPkt(dstIP);
    }
  }

  void _verifyHelper() {
    std::vector<int64_t> beforeQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      beforeQueueOutPkts.push_back(
          this->getLatestPortStats(this->masterLogicalPortIds()[0])
              .get_queueOutPackets_()
              .at(queueId));
    }

    this->sendUdpPkts();

    std::vector<int64_t> afterQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      afterQueueOutPkts.push_back(
          this->getLatestPortStats(this->masterLogicalPortIds()[0])
              .get_queueOutPackets_()
              .at(queueId));
    }

    for (auto i = 0; i < beforeQueueOutPkts.size(); i++) {
      EXPECT_EQ(afterQueueOutPkts.at(i) - beforeQueueOutPkts.at(i), 1);
    }
  }

 private:
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
};

using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_SUITE(HwQueuePerHostTest, TestTypes);

TYPED_TEST(HwQueuePerHostTest, VerifyHostToQueueMappingClassIDsAfterResolve) {
  if (!this->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [this] {
    this->setupECMPForwarding();

    /*
     * Resolve neighbors, then apply classID
     * Prod will typically follow this sequence as LookupClassUpdater is
     * implemented as a state observer which would update resolved neighbors
     * with classIDs.
     */
    auto state1 = this->addNeighbors(this->getProgrammedState());
    auto state2 = this->resolveNeighbors(state1);
    this->applyNewState(state2);

    auto state3 = this->updateClassID(this->getProgrammedState());
    this->applyNewState(state3);
  };

  auto verify = [=]() { this->_verifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwQueuePerHostTest, VerifyHostToQueueMappingClassIDsWithResolve) {
  if (!this->isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  /*
   * Resolve neighbors + apply classID together.
   */
  auto setup = [this] {
    this->setupECMPForwarding();

    auto state1 = this->addNeighbors(this->getProgrammedState());
    auto state2 = this->resolveNeighbors(state1);
    auto state3 = this->updateClassID(state2);

    this->applyNewState(state3);
  };

  auto verify = [=]() { this->_verifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
