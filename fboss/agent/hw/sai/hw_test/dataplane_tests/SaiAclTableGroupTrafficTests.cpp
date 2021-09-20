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
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include "fboss/agent/hw/test/HwTestAclUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss {

class SaiAclTableGroupTrafficTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    FLAGS_enable_acl_table_group = true;
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    utility::addQueuePerHostQueueConfig(&cfg);
    utility::addQueuePerHostAclTables(&cfg);
    return cfg;
  }

  cfg::AclStage kAclStage() const {
    return cfg::AclStage::INGRESS;
  }

  template <typename AddrT>
  const std::map<AddrT, std::pair<folly::MacAddress, cfg::AclLookupClass>>&
  getIpToMacAndClassID() {
    // TODO (skhare) Use ResourceGenerator to create this map, where the number
    // of entries equals kQueuePerhostQueueIds()

    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      static const std::map<
          folly::IPAddressV4,
          std::pair<folly::MacAddress, cfg::AclLookupClass>>
          ipToMacAndClassID = {
              {folly::IPAddressV4("1.0.0.10"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:10"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0)},
              {folly::IPAddressV4("1.0.0.11"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:11"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1)},
              {folly::IPAddressV4("1.0.0.12"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:12"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2)},
              {folly::IPAddressV4("1.0.0.13"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:13"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3)},
              {folly::IPAddressV4("1.0.0.14"),
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
              {folly::IPAddressV6("1::10"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:10"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0)},
              {folly::IPAddressV6("1::11"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:11"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1)},
              {folly::IPAddressV6("1::12"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:12"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2)},
              {folly::IPAddressV6("1::13"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:13"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3)},
              {folly::IPAddressV6("1::14"),
               std::make_pair(
                   folly::MacAddress("0:2:3:4:5:14"),
                   cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4)},
          };
      return ipToMacAndClassID;
    }
  }

  bool isSupported() const {
    return HwTest::isSupported(HwAsic::Feature::L3_QOS);
  }

  template <typename AddrT>
  std::shared_ptr<SwitchState> addNeighbors(
      const std::shared_ptr<SwitchState>& inState) {
    using NeighborTableT = typename std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    auto outState{inState->clone()};

    for (const auto& ipToMacAndClassID : getIpToMacAndClassID<AddrT>()) {
      auto ip = ipToMacAndClassID.first;
      auto neighborTable = outState->getVlans()
                               ->getVlan(kVlanID)
                               ->template getNeighborTable<NeighborTableT>()
                               ->modify(kVlanID, &outState);
      neighborTable->addPendingEntry(ip, kIntfID);
    }

    return outState;
  }

  template <typename AddrT>
  std::shared_ptr<SwitchState> updateNeighbors(
      const std::shared_ptr<SwitchState>& inState,
      bool setClassIDs) {
    using NeighborTableT = typename std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    auto outState{inState->clone()};

    for (const auto& ipToMacAndClassID : getIpToMacAndClassID<AddrT>()) {
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

  template <typename AddrT>
  std::shared_ptr<SwitchState> resolveNeighbors(
      const std::shared_ptr<SwitchState>& inState) {
    return updateNeighbors<AddrT>(inState, false /* setClassIDs */);
  }

  template <typename AddrT>
  std::shared_ptr<SwitchState> updateClassID(
      const std::shared_ptr<SwitchState>& inState) {
    return updateNeighbors<AddrT>(inState, true /* setClassIDs */);
  }

  template <typename AddrT>
  void sendPacketHelper(bool frontPanel) {
    for (const auto& ipToMacAndClassID : getIpToMacAndClassID<AddrT>()) {
      auto dstIP = ipToMacAndClassID.first;
      sendPacket<AddrT>(dstIP, frontPanel, 64 /* ttl < 128 */);
      sendPacket<AddrT>(dstIP, frontPanel, 128 /* ttl >= 128 */);
    }
  }

  template <typename AddrT>
  void _verifyHelperMultipleAclTables(bool frontPanel) {
    std::string testType;

    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      testType = "IPv4 Traffic";
    } else if constexpr (std::is_same<AddrT, folly::IPAddressV6>::value) {
      testType = "IPv6 Traffic";
    }

    auto ttlAclName = utility::getQueuePerHostTtlAclName();
    auto ttlCounterName = utility::getQueuePerHostTtlCounterName();

    auto statBefore = utility::getAclInOutPackets(
        getHwSwitch(),
        this->getProgrammedState(),
        ttlAclName,
        ttlCounterName,
        kAclStage(),
        utility::getTtlAclTableName());

    std::map<int, int64_t> beforeQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      beforeQueueOutPkts[queueId] =
          this->getLatestPortStats(this->masterLogicalPortIds()[0])
              .get_queueOutPackets_()
              .at(queueId);
    }

    for (const auto& ipToMacAndClassID : getIpToMacAndClassID<AddrT>()) {
      auto dstIP = ipToMacAndClassID.first;
      sendPacket<AddrT>(dstIP, frontPanel, 64 /* ttl < 128 */);
      sendPacket<AddrT>(dstIP, frontPanel, 128 /* ttl >= 128 */);
    }

    std::map<int, int64_t> afterQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      afterQueueOutPkts[queueId] =
          this->getLatestPortStats(this->masterLogicalPortIds()[0])
              .get_queueOutPackets_()
              .at(queueId);
    }

    /*
     *  Consider ACL with action to egress pkts through queue 2.
     *
     *  CPU originated packets:
     *     - Hits ACL (queue2Cnt = 1), egress through queue 2 of port0.
     *     - port0 is in loopback mode, so the packet gets looped back.
     *     - When packet is routed, its dstMAC gets overwritten. Thus, the
     *       looped back packet is not routed, and thus does not hit the ACL.
     *     - On some platforms, looped back packets for unknown MACs are
     *       flooded and counted on queue *before* the split horizon check
     *       (drop when srcPort == dstPort). This flooding always happens on
     *       queue 0, so expect one or more packets on queue 0.
     *
     *  Front panel packets (injected with pipeline bypass):
     *     - Egress out of port1 queue0 (pipeline bypass).
     *     - port1 is in loopback mode, so the packet gets looped back.
     *     - Rest of the workflow is same as above when CPU originated packet
     *       gets injected for switching.
     */
    for (auto [qid, beforePkts] : beforeQueueOutPkts) {
      auto pktsOnQueue = afterQueueOutPkts[qid] - beforePkts;
      XLOG(INFO) << "TestType: " << testType << " Pkts on queue : " << qid
                 << " pkts: " << pktsOnQueue;
      if (qid == 0) {
        EXPECT_GE(pktsOnQueue, 1);
      } else {
        EXPECT_EQ(
            pktsOnQueue, 2 /* 1 pkt each for ttl < 128 and ttl >= 128 */
        );
      }
    }

    auto statAfter = utility::getAclInOutPackets(
        getHwSwitch(),
        this->getProgrammedState(),
        ttlAclName,
        ttlCounterName,
        kAclStage(),
        utility::getTtlAclTableName());

    // counts ttl >= 128 packet only
    EXPECT_EQ(statAfter - statBefore, getIpToMacAndClassID<AddrT>().size());
  }

  template <typename AddrT>
  std::shared_ptr<SwitchState> addResolvedNeighborWithClassID(
      const std::shared_ptr<SwitchState>& inState) {
    auto state1 = addNeighbors<AddrT>(inState);
    auto state2 = resolveNeighbors<AddrT>(state1);
    auto state3 = updateClassID<AddrT>(state2);

    return state3;
  }

  void verifyMultipleAclTablesHelper(bool frontPanel) {
    ASSERT_TRUE(HwTest::isSupported(HwAsic::Feature::MULTIPLE_ACL_TABLES));

    auto setup = [this]() {
      resolveNeigborAndProgramRoutes(*helper_, kEcmpWidth);

      auto state1 = addResolvedNeighborWithClassID<folly::IPAddressV4>(
          this->getProgrammedState());
      auto state2 = addResolvedNeighborWithClassID<folly::IPAddressV6>(state1);
      applyNewState(state2);
    };

    auto verify = [this, frontPanel]() {
      _verifyHelperMultipleAclTables<folly::IPAddressV4>(frontPanel);
      _verifyHelperMultipleAclTables<folly::IPAddressV6>(frontPanel);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  template <typename AddrT>
  AddrT kSrcIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("1.0.0.1");
    } else {
      return folly::IPAddressV6("1::1");
    }
  }

 private:
  template <typename AddrT>
  void sendPacket(AddrT dstIP, bool frontPanel, uint8_t ttl) {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        kSrcIP<AddrT>(),
        dstIP,
        8000, // l4 src port
        8001, // l4 dst port
        48 << 2, // DSCP
        ttl);
    // port is in LB mode, so it will egress and immediately loop back.
    // Since it is not re-written, it should hit the pipeline as if it
    // ingressed on the port, and be properly queued.
    if (frontPanel) {
      auto outPort = helper_->ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), outPort);
    } else {
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
    }
  }

  static inline constexpr auto kEcmpWidth = 1;
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

TEST_F(
    SaiAclTableGroupTrafficTest,
    VerifyQueuePerHostAclTableAndTtlAclTableFrontPanel) {
  if (!isSupported()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  verifyMultipleAclTablesHelper(false /* cpu port */);
}

TEST_F(
    SaiAclTableGroupTrafficTest,
    VerifyQueuePerHostAclTableAndTtlAclTableCpu) {
  if (!isSupported()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  verifyMultipleAclTablesHelper(true /* cpu port */);
}

} // namespace facebook::fboss
