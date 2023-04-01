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
#include "fboss/agent/hw/test/dataplane_tests/HwTestDscpMarkingUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
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
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());

    utility::addAclTableGroup(
        &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());

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

  const std::vector<uint8_t> ttlfields = {64, 128};
  /* Default Dscp Value used before */
  const uint8_t dscpDefault = 48;

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
            NeighborState::REACHABLE,
            classID);

      } else {
        neighborTable->updateEntry(
            ip,
            neighborMac,
            PortDescriptor(masterLogicalPortIds()[0]),
            kIntfID,
            NeighborState::REACHABLE);
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
  void sendPacketHelper(bool frontPanel, uint8_t dscp) {
    for (const auto& ipToMacAndClassID : getIpToMacAndClassID<AddrT>()) {
      auto dstIP = ipToMacAndClassID.first;
      sendPacket<AddrT>(
          dstIP,
          frontPanel,
          ttlfields[0] /* ttl < 128 */,
          dscp,
          IP_PROTO::IP_PROTO_UDP,
          std::nullopt,
          std::nullopt);
      sendPacket<AddrT>(
          dstIP,
          frontPanel,
          ttlfields[1] /* ttl >= 128 */,
          dscp,
          IP_PROTO::IP_PROTO_UDP,
          std::nullopt,
          std::nullopt);
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
      sendPacket<AddrT>(
          dstIP,
          frontPanel,
          ttlfields[0] /* ttl < 128 */,
          dscpDefault,
          IP_PROTO::IP_PROTO_UDP,
          std::nullopt,
          std::nullopt);
      sendPacket<AddrT>(
          dstIP,
          frontPanel,
          ttlfields[1] /* ttl >= 128 */,
          dscpDefault,
          IP_PROTO::IP_PROTO_UDP,
          std::nullopt,
          std::nullopt);
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
      XLOG(DBG2) << "TestType: " << testType << " Pkts on queue : " << qid
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
    bool multipleAclTableSupport =
        HwTest::isSupported(HwAsic::Feature::MULTIPLE_ACL_TABLES);
#if defined(TAJO_SDK_VERSION_1_42_1) || defined(TAJO_SDK_VERSION_1_42_8)
    multipleAclTableSupport = false;
#endif
    ASSERT_TRUE(multipleAclTableSupport);

    auto setup = [this]() {
      /*
       * Tajo Asic needs a key profile to be set which is supposed to be a
       * superset of all the qualifiers/action types of all the tables. If key
       * profile is absent, the first table's attributes will be taken as the
       * key profile. Hence, the first table is always set with the superset of
       * qualifiers. addTtlQualifier is used in Tajo SDK versions which support
       * Multi ACL table to add the superset of qualifiers/action types in first
       * table
       */
      bool addTtlQualifier = false;
      resolveNeigborAndProgramRoutes(*helper_, kEcmpWidth);
#if defined(TAJO_SDK_VERSION_1_62_0)
      addTtlQualifier = true;
#endif

      auto state1 = addResolvedNeighborWithClassID<folly::IPAddressV4>(
          this->getProgrammedState());
      auto state2 = addResolvedNeighborWithClassID<folly::IPAddressV6>(state1);
      applyNewState(state2);

      if (isSupported()) {
        auto newCfg{initialConfig()};
        utility::addQueuePerHostQueueConfig(&newCfg);
        utility::addQueuePerHostAclTables(
            &newCfg, 1 /*priority*/, addTtlQualifier);
        utility::addTtlAclTable(&newCfg, 2 /*priority*/);
        applyNewConfig(newCfg);
      }
    };

    auto verify = [this, frontPanel]() {
      _verifyHelperMultipleAclTables<folly::IPAddressV4>(frontPanel);
      _verifyHelperMultipleAclTables<folly::IPAddressV6>(frontPanel);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  template <typename AddrT>
  std::pair<std::string, AddrT> testTypeAndIpHelper() {
    std::string testType;
    AddrT dstIP;

    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      testType = "IPv4 Traffic";
      dstIP = folly::IPAddressV4("1.0.0.10");
    } else if constexpr (std::is_same<AddrT, folly::IPAddressV6>::value) {
      testType = "IPv6 Traffic";
      dstIP = folly::IPAddressV6("1::10");
    }

    return std::make_pair(testType, dstIP);
  }

  std::pair<uint64_t, uint64_t> pktCounterHelper() {
    auto kDscpCounterAclName = utility::kDscpCounterAclName();
    auto kCounterName = utility::kCounterName();
    auto dscpAclPkts = utility::getAclInOutPackets(
        getHwSwitch(),
        this->getProgrammedState(),
        kDscpCounterAclName,
        kCounterName,
        kAclStage(),
        utility::getDscpAclTableName());

    auto ttlAclName = utility::getQueuePerHostTtlAclName();
    auto ttlCounterName = utility::getQueuePerHostTtlCounterName();
    auto ttlAclPkts = utility::getAclInOutPackets(
        getHwSwitch(),
        this->getProgrammedState(),
        ttlAclName,
        ttlCounterName,
        kAclStage(),
        utility::getTtlAclTableName());

    return std::make_pair(dscpAclPkts, ttlAclPkts);
  }

  template <typename AddrT>
  void _verifyHelperDscpTtlAclTables(bool frontPanel) {
    auto [testType, dstIP] = testTypeAndIpHelper<AddrT>();

    auto [beforeDscpAclPkts, beforeTtlAclPkts] = pktCounterHelper();
    sendAllPacketshelper<AddrT>(dstIP, frontPanel, 0);
    auto [intermediateDscpAclPkts, intermediateTtlAclPkts] = pktCounterHelper();
    sendAllPacketshelper<AddrT>(
        dstIP, frontPanel, utility::kIcpDscp(getAsic()));
    auto [afterDscpAclPkts, afterTtlAclPkts] = pktCounterHelper();

    XLOG(DBG2) << "TestType: " << testType;
    XLOG(DBG2) << "Before ICP pkts: " << beforeDscpAclPkts
               << " Intermediate ICP pkts: " << intermediateDscpAclPkts
               << " After ICP pkts: " << afterDscpAclPkts;
    XLOG(DBG2) << "Before Ttl pkts: " << beforeTtlAclPkts
               << " Intermediate Ttl pkts: " << intermediateTtlAclPkts
               << " After Ttl pkts: " << afterTtlAclPkts;

    /* When packet egresses, DSCP is not marked and so ACL counter is not hit.
     * It marks the DSCP instead. Since the port is in loopback mode, the same
     * packet comes in and this time hits the counter and is incremented
     */
    EXPECT_EQ(
        intermediateDscpAclPkts - beforeDscpAclPkts,
        (1 /* ACL hit once */ * 2 /* Pkt sent twice with different TTL */ *
         2 /*l4Srcport and l4DstPort */ *
         (utility::kUdpPorts().size() +
          utility::kTcpPorts()
              .size()) /* For each destIP, all ICP port pkts are sent */));

    EXPECT_EQ(
        intermediateTtlAclPkts - beforeTtlAclPkts,
        (2 /*l4Srcport and l4DstPort */ *
         (utility::kUdpPorts().size() + utility::kTcpPorts().size())));

    /* counts ttl >= 128 packet only. We send twice with and without DSCP
     * marking. Each time, we send to a list of dest ports.
     * For each send, we send for all UDP and TCP ICP ports. so the expected
     * value needs to account for that.
     */
    EXPECT_EQ(
        afterTtlAclPkts - intermediateTtlAclPkts,
        (2 /*l4Srcport and l4DstPort */ *
         (utility::kUdpPorts().size() + utility::kTcpPorts().size())));

    /* Inject a pkt with dscp = ICP DSCP.
     *   - The packet will match DSCP ACL, thus counter incremented.
     *   - Packet egress via front panel port which is in loopback mode.
     *   - Thus, packet gets looped back.
     *   - Hits ACL again, and thus counter incremented twice.
     */
    EXPECT_EQ(
        afterDscpAclPkts - intermediateDscpAclPkts,
        (2 /* ACL hit twice */ * 2 /* Pkt sent twice with different TTL */ *
         2 /*L4Srcport and L4DstPort */ *
         (utility::kUdpPorts().size() + utility::kTcpPorts().size())));
  }

  void verifyDscpTtlAclTablesHelper(bool frontPanel) {
    bool multipleAclTableSupport =
        HwTest::isSupported(HwAsic::Feature::MULTIPLE_ACL_TABLES);
#if defined(TAJO_SDK_VERSION_1_42_1) || defined(TAJO_SDK_VERSION_1_42_8)
    multipleAclTableSupport = false;
#endif
    ASSERT_TRUE(multipleAclTableSupport);

    auto setup = [this]() {
      /*
       * Refer to the detailed comment in setup of verifyMultipleAclTablesHelper
       * for the reason behind this flag
       */
      bool addTtlQualifier = false;
      resolveNeigborAndProgramRoutes(*helper_, kEcmpWidth);
#if defined(TAJO_SDK_VERSION_1_62_0)
      addTtlQualifier = true;
#endif

      auto state1 = addResolvedNeighborWithClassID<folly::IPAddressV4>(
          this->getProgrammedState());
      auto state2 = addResolvedNeighborWithClassID<folly::IPAddressV6>(state1);
      applyNewState(state2);

      if (isSupported()) {
        auto newCfg{initialConfig()};
        utility::addOlympicQosMaps(newCfg, getAsic());
        utility::addDscpAclTable(
            &newCfg, 1 /*priority*/, addTtlQualifier, getAsic());
        utility::addTtlAclTable(&newCfg, 2);
        applyNewConfig(newCfg);
      }
    };

    auto verify = [this, frontPanel]() {
      // TODO: IPV4 not working. It needs to be triaged and fixed
      //_verifyHelperDscpTtlAclTables<folly::IPAddressV4>(frontPanel);
      _verifyHelperDscpTtlAclTables<folly::IPAddressV6>(frontPanel);
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
  void sendAllPacketshelper(AddrT dstIP, bool frontPanel, uint8_t dscp) {
    sendAllPackets(
        dstIP, frontPanel, dscp, IP_PROTO::IP_PROTO_UDP, utility::kUdpPorts());
    sendAllPackets(
        dstIP, frontPanel, dscp, IP_PROTO::IP_PROTO_TCP, utility::kTcpPorts());
  }
  template <typename AddrT>
  void sendAllPackets(
      AddrT dstIP,
      bool frontPanel,
      uint8_t dscp,
      IP_PROTO proto,
      const std::vector<uint32_t>& ports) {
    for (auto ttl : ttlfields) {
      for (auto port : ports) {
        sendPacket(
            dstIP,
            frontPanel,
            ttl,
            dscp,
            proto,
            port /* l4SrcPort */,
            std::nullopt /* l4DstPort */);
        sendPacket(
            dstIP,
            frontPanel,
            ttl,
            dscp,
            proto,
            std::nullopt /* l4SrcPort */,
            port /* l4DstPort */);
      }
    }
  }

  template <typename AddrT>
  void sendPacket(
      AddrT dstIP,
      bool frontPanel,
      uint8_t ttl,
      uint8_t dscp,
      IP_PROTO proto,
      std::optional<uint16_t> l4SrcPort,
      std::optional<uint16_t> l4DstPort) {
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    std::unique_ptr<facebook::fboss::TxPacket> txPacket;
    CHECK(proto == IP_PROTO::IP_PROTO_UDP || proto == IP_PROTO::IP_PROTO_TCP);
    if (proto == IP_PROTO::IP_PROTO_UDP) {
      txPacket = utility::makeUDPTxPacket(
          getHwSwitch(),
          vlanId,
          srcMac, // src mac
          intfMac, // dst mac
          kSrcIP<AddrT>(),
          dstIP,
          l4SrcPort.has_value() ? l4SrcPort.value() : 8000,
          l4DstPort.has_value() ? l4DstPort.value() : 8001,
          dscp << 2,
          ttl);
    } else if (proto == IP_PROTO::IP_PROTO_TCP) {
      txPacket = utility::makeTCPTxPacket(
          getHwSwitch(),
          vlanId,
          srcMac, // src mac
          intfMac, // dst mac
          kSrcIP<AddrT>(),
          dstIP,
          l4SrcPort.has_value() ? l4SrcPort.value() : 8000,
          l4DstPort.has_value() ? l4DstPort.value() : 8001,
          dscp << 2,
          ttl);
    }
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

TEST_F(SaiAclTableGroupTrafficTest, VerifyDscpMarkingAndTtlAclTableCpu) {
  if (!isSupported()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  verifyDscpTtlAclTablesHelper(true /* cpu port */);
}

TEST_F(SaiAclTableGroupTrafficTest, VerifyDscpMarkingAndTtlAclTableFrontPanel) {
  if (!isSupported()) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  verifyDscpTtlAclTablesHelper(false /* cpu port */);
}

} // namespace facebook::fboss
