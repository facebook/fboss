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

namespace facebook::fboss {

template <typename AddrT>
class HwQueuePerHostTest : public HwLinkStateDependentTest {
  using NeighborTableT = typename std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode());
    return cfg;
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
      bool setClassIDs,
      bool blockNeighbor) {
    auto outState{inState->clone()};

    for (const auto& ipToMacAndClassID : getIpToMacAndClassID()) {
      auto ip = ipToMacAndClassID.first;
      auto macAndClassID = ipToMacAndClassID.second;
      auto neighborMac = macAndClassID.first;
      auto classID = blockNeighbor ? cfg::AclLookupClass::CLASS_DROP
                                   : macAndClassID.second;
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

  std::shared_ptr<SwitchState> resolveNeighbors(
      const std::shared_ptr<SwitchState>& inState) {
    return updateNeighbors(
        inState, false /* setClassIDs */, false /* blockNeighbors */);
  }

  std::shared_ptr<SwitchState> updateClassID(
      const std::shared_ptr<SwitchState>& inState,
      bool blockNeighbor) {
    return updateNeighbors(inState, true /* setClassIDs */, blockNeighbor);
  }

  void _setupHelper() {
    resolveNeigborAndProgramRoutes(*helper_, kEcmpWidth);
    auto newCfg{initialConfig()};
    utility::addQueuePerHostQueueConfig(&newCfg);
    utility::addQueuePerHostAcls(&newCfg);
    applyNewConfig(newCfg);
  }

  void _verifyHelper(bool frontPanel, bool blockNeighbor) {
    auto ttlAclName = utility::getQueuePerHostTtlAclName();
    auto ttlCounterName = utility::getQueuePerHostTtlCounterName();

    auto statBefore = utility::getAclInOutPackets(
        getHwSwitch(), this->getProgrammedState(), ttlAclName, ttlCounterName);

    std::map<int, int64_t> beforeQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      beforeQueueOutPkts[queueId] =
          this->getLatestPortStats(this->masterLogicalPortIds()[0])
              .get_queueOutPackets_()
              .at(queueId);
    }

    for (const auto& ipToMacAndClassID : getIpToMacAndClassID()) {
      auto dstIP = ipToMacAndClassID.first;
      sendPacket(dstIP, frontPanel, 64 /* ttl < 128 */);
      sendPacket(dstIP, frontPanel, 128 /* ttl >= 128 */);
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
      XLOG(DBG2) << " Pkts on queue : " << qid << " pkts: " << pktsOnQueue;

      if (blockNeighbor) {
        // if the neighbor is blocked, all pkts are dropped
        EXPECT_EQ(pktsOnQueue, 0);
      } else {
        if (qid == 0) {
          EXPECT_GE(pktsOnQueue, 1);
        } else {
          EXPECT_EQ(
              pktsOnQueue, 2 /* 1 pkt each for ttl < 128 and ttl >= 128 */);
        }
      }
    }

    auto statAfter = utility::getAclInOutPackets(
        getHwSwitch(), this->getProgrammedState(), ttlAclName, ttlCounterName);

    if (blockNeighbor) {
      // if the neighbor is blocked, all pkts are dropped
      EXPECT_EQ(statAfter - statBefore, 0);
    } else {
      // counts ttl >= 128 packet only
      EXPECT_EQ(statAfter - statBefore, getIpToMacAndClassID().size());
    }
  }

  void classIDAfterNeighborResolveHelper(bool blockNeighbor) {
    this->_setupHelper();

    /*
     * Resolve neighbors, then apply classID
     * Prod will typically follow this sequence as LookupClassUpdater is
     * implemented as a state observer which would update resolved neighbors
     * with classIDs.
     */
    auto state1 = this->addNeighbors(this->getProgrammedState());
    auto state2 = this->resolveNeighbors(state1);
    this->applyNewState(state2);

    auto state3 =
        this->updateClassID(this->getProgrammedState(), blockNeighbor);
    this->applyNewState(state3);
  }

  void classIDWithResolveHelper(bool blockNeighbor) {
    this->_setupHelper();

    auto state1 = this->addNeighbors(this->getProgrammedState());
    auto state2 = this->resolveNeighbors(state1);
    auto state3 = this->updateClassID(state2, blockNeighbor);

    this->applyNewState(state3);
  }

  void verifyHostToQueueMappingClassIDsAfterResolveHelper(
      bool frontPanel,
      bool blockNeighbor) {
    auto setup = [this, blockNeighbor]() {
      this->classIDAfterNeighborResolveHelper(blockNeighbor);
    };
    auto verify = [this, frontPanel, blockNeighbor]() {
      this->_verifyHelper(frontPanel, blockNeighbor);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void verifyHostToQueueMappingClassIDsWithResolveHelper(
      bool frontPanel,
      bool blockNeighbor) {
    auto setup = [this, blockNeighbor]() {
      this->classIDWithResolveHelper(blockNeighbor);
    };
    auto verify = [this, frontPanel, blockNeighbor]() {
      this->_verifyHelper(frontPanel, blockNeighbor);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void verifyTtldCounter(bool frontPanel) {
    auto setup = [this]() {
      this->_setupHelper();

      auto state1 = this->addNeighbors(this->getProgrammedState());
      auto state2 = this->resolveNeighbors(state1);

      this->applyNewState(state2);
    };

    auto verify = [this, frontPanel]() {
      auto ttlAclName = utility::getQueuePerHostTtlAclName();
      auto ttlCounterName = utility::getQueuePerHostTtlCounterName();

      auto packetsBefore = utility::getAclInOutPackets(
          getHwSwitch(),
          this->getProgrammedState(),
          ttlAclName,
          ttlCounterName);

      auto bytesBefore = utility::getAclInOutBytes(
          getHwSwitch(),
          this->getProgrammedState(),
          ttlAclName,
          ttlCounterName);

      auto dstIP = getIpToMacAndClassID().begin()->first;

      sendPacket(dstIP, frontPanel, 64 /* ttl < 128 */);
      size_t packetSize = sendPacket(dstIP, frontPanel, 128 /* ttl >= 128 */);

      auto packetsAfter = utility::getAclInOutPackets(
          getHwSwitch(),
          this->getProgrammedState(),
          ttlAclName,
          ttlCounterName);

      auto bytesAfter = utility::getAclInOutBytes(
          getHwSwitch(),
          this->getProgrammedState(),
          ttlAclName,
          ttlCounterName);

      XLOG(DBG2) << "\n"
                 << "ttlAclPacketCounter: " << std::to_string(packetsBefore)
                 << " -> " << std::to_string(packetsAfter) << "\n"
                 << "ttlAclBytesCounter: " << std::to_string(bytesBefore)
                 << " -> " << std::to_string(bytesAfter);

      // counts ttl >= 128 packet only
      EXPECT_EQ(packetsAfter - packetsBefore, 1);
      if (frontPanel) {
        EXPECT_EQ(bytesAfter - bytesBefore, packetSize);
      } else {
        // TODO: Still need to debug why we get extra 4 bytes for CPU port
        EXPECT_GE(bytesAfter - bytesBefore, packetSize);
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  AddrT kSrcIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("1.0.0.1");
    } else {
      return folly::IPAddressV6("1::1");
    }
  }

 private:
  size_t sendPacket(AddrT dstIP, bool frontPanel, uint8_t ttl) {
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        kSrcIP(),
        dstIP,
        8000, // l4 src port
        8001, // l4 dst port
        48 << 2, // DSCP
        ttl);

    size_t txPacketSize = txPacket->buf()->length();
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

    return txPacketSize;
  }

  static inline constexpr auto kEcmpWidth = 1;
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_SUITE(HwQueuePerHostTest, TestTypes);

// Verify that traffic arriving on a front panel port gets right queue-per-host
// queue.
TYPED_TEST(
    HwQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsAfterResolveFrontPanel) {
  this->verifyHostToQueueMappingClassIDsAfterResolveHelper(
      true /* front panel port */, false /* block neighbor */);
}

// Verify that traffic arriving on a front panel port to a blocked neighbor gets
// dropped.
TYPED_TEST(
    HwQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsAfterResolveFrontPanelBlock) {
  this->verifyHostToQueueMappingClassIDsAfterResolveHelper(
      true /* front panel port */, true /* block neighbor */);
}

// Verify that traffic originating on the CPU is gets right queue-per-host
// queue.
TYPED_TEST(
    HwQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsAfterResolveCpu) {
  this->verifyHostToQueueMappingClassIDsAfterResolveHelper(
      false /* cpu port */, false /* block neighbor */);
}

// Verify that traffic originating on the CPU to a blocked neighbor gets
// dropped.
TYPED_TEST(
    HwQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsAfterResolveCpuBlock) {
  this->verifyHostToQueueMappingClassIDsAfterResolveHelper(
      false /* cpu port */, true /* block neighbor */);
}

// Verify that traffic arriving on a front panel port gets right queue-per-host
// queue.
TYPED_TEST(
    HwQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsWithResolveFrontPanel) {
  this->verifyHostToQueueMappingClassIDsWithResolveHelper(
      true /* front panel port */, false /* block neighbor */);
}

// Verify that traffic arriving on a front panel port to a blocked neighbor gets
// dropped.
TYPED_TEST(
    HwQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsWithResolveFrontPanelBlock) {
  this->verifyHostToQueueMappingClassIDsWithResolveHelper(
      true /* front panel port */, true /* block neighbor */);
}

// Verify that traffic originating on the CPU is gets right queue-per-host
// queue.
TYPED_TEST(HwQueuePerHostTest, VerifyHostToQueueMappingClassIDsWithResolveCpu) {
  this->verifyHostToQueueMappingClassIDsWithResolveHelper(
      false /* cpu port */, false /* block neighbor */);
}

// Verify that traffic originating on the CPU to a blocked neighbor gets
// dropped.
TYPED_TEST(
    HwQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsWithResolveCpuBlock) {
  this->verifyHostToQueueMappingClassIDsWithResolveHelper(
      false /* cpu port */, true /* block neighbor */);
}

// Verify that TTLd traffic not going to queue-per-host has TTLd counter
// incremented.
TYPED_TEST(HwQueuePerHostTest, VerifyTtldCounterFrontPanel) {
  this->verifyTtldCounter(true /* front panel port */);
}

// Verify that TTLd traffic not going to queue-per-host has TTLd counter
// incremented.
TYPED_TEST(HwQueuePerHostTest, VerifyTtldCounterCpu) {
  this->verifyTtldCounter(false /* cpu port */);
}

} // namespace facebook::fboss
