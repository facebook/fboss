/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/NeighborTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

DECLARE_bool(intf_nbr_tables);

namespace facebook::fboss {

template <typename AddrType, bool enableIntfNbrTable>
struct IpAddrAndEnableIntfNbrTableT {
  using AddrT = AddrType;
  static constexpr auto intfNbrTable = enableIntfNbrTable;
};

using TestTypes = ::testing::Types<
    IpAddrAndEnableIntfNbrTableT<folly::IPAddressV4, false>,
    IpAddrAndEnableIntfNbrTableT<folly::IPAddressV4, true>,
    IpAddrAndEnableIntfNbrTableT<folly::IPAddressV6, false>,
    IpAddrAndEnableIntfNbrTableT<folly::IPAddressV6, true>>;

template <typename IpAddrAndEnableIntfNbrTableT>
class AgentQueuePerHostTest : public AgentHwTest {
  using AddrT = typename IpAddrAndEnableIntfNbrTableT::AddrT;
  static auto constexpr isIntfNbrTable =
      IpAddrAndEnableIntfNbrTableT::intfNbrTable;
  using NeighborTableT = typename std::conditional_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  void setCmdLineFlagOverrides() const override {
    FLAGS_intf_nbr_tables = isIntfNbrTable;
    AgentHwTest::setCmdLineFlagOverrides();
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addQueuePerHostQueueConfig(&cfg);
    utility::addQueuePerHostAcls(&cfg, ensemble.isSai());
    return cfg;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::QUEUE_PER_HOST};
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

      NeighborTableT* neighborTable;
      if (isIntfNbrTable) {
        neighborTable = outState->getInterfaces()
                            ->getNode(kIntfID)
                            ->template getNeighborTable<NeighborTableT>()
                            ->modify(kIntfID, &outState);
      } else {
        neighborTable = outState->getVlans()
                            ->getNode(kVlanID)
                            ->template getNeighborTable<NeighborTableT>()
                            ->modify(kVlanID, &outState);
      }

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

      NeighborTableT* neighborTable;
      if (isIntfNbrTable) {
        neighborTable = outState->getInterfaces()
                            ->getNode(kIntfID)
                            ->template getNeighborTable<NeighborTableT>()
                            ->modify(kIntfID, &outState);
      } else {
        neighborTable = outState->getVlans()
                            ->getNode(kVlanID)
                            ->template getNeighborTable<NeighborTableT>()
                            ->modify(kVlanID, &outState);
      }

      auto existingEntry = neighborTable->getEntryIf(ip);

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

      if (getSw()->needL2EntryForNeighbor()) {
        outState = utility::NeighborTestUtils::updateMacEntryForUpdatedNbrEntry(
            outState, kVlanID, existingEntry, neighborTable->getEntryIf(ip));
      }
    }

    return outState;
  }

  void verifyNeighborClassId(bool blockNeighbor) {
    WITH_RETRIES({
      auto state = getProgrammedState();
      for (const auto& ipToMacAndClassID : getIpToMacAndClassID()) {
        auto ip = ipToMacAndClassID.first;
        auto macAndClassID = ipToMacAndClassID.second;
        auto classID = blockNeighbor ? cfg::AclLookupClass::CLASS_DROP
                                     : macAndClassID.second;

        std::shared_ptr<NeighborTableT> neighborTable;
        if (isIntfNbrTable) {
          neighborTable = state->getInterfaces()
                              ->getNode(kIntfID)
                              ->template getNeighborTable<NeighborTableT>();
        } else {
          neighborTable = state->getVlans()
                              ->getNode(kVlanID)
                              ->template getNeighborTable<NeighborTableT>();
        }

        auto entry = neighborTable->getEntryIf(ip);
        XLOG(DBG2) << "Verify class id for " << ip
                   << " expected classID: " << static_cast<int>(classID)
                   << " found " << static_cast<int>(*entry->getClassID());
        EXPECT_EVENTUALLY_EQ(entry->getClassID(), classID);
      }
    });
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

  void setMacAddrsToBlock() {
    auto cfgMacAddrsToBlock = std::make_unique<std::vector<cfg::MacAndVlan>>();
    for (const auto& ipToMacAndClassID : getIpToMacAndClassID()) {
      auto macAndClassID = ipToMacAndClassID.second;
      auto macAddress = macAndClassID.first;
      cfg::MacAndVlan macAndVlan;
      macAndVlan.vlanID() = kVlanID;
      macAndVlan.macAddress() = macAddress.toString();
      cfgMacAddrsToBlock->emplace_back(macAndVlan);
    }
    ThriftHandler handler(getSw());
    handler.setMacAddrsToBlock(std::move(cfgMacAddrsToBlock));
  }

  void _verifyHelper(bool frontPanel, bool blockNeighbor) {
    XLOG(DBG2) << "verify send packets "
               << (frontPanel ? "out of port" : "switched");
    // wait for pending state updates to complete
    waitForStateUpdates(getSw());
    verifyNeighborClassId(blockNeighbor);
    auto ttlAclName = utility::getQueuePerHostTtlAclName();
    auto ttlCounterName = utility::getQueuePerHostTtlCounterName();

    auto statBefore = utility::getAclInOutPackets(getSw(), ttlCounterName);

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

    WITH_RETRIES({
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
          EXPECT_EVENTUALLY_EQ(pktsOnQueue, 0);
        } else {
          if (qid == 0) {
            EXPECT_EVENTUALLY_GE(pktsOnQueue, 1);
          } else {
            EXPECT_EVENTUALLY_EQ(
                pktsOnQueue, 2 /* 1 pkt each for ttl < 128 and ttl >= 128 */);
          }
        }
      }
    });

    auto aclStatsMatch = [&]() {
      auto statAfter = utility::getAclInOutPackets(getSw(), ttlCounterName);
      XLOG(DBG2) << " Acl stats : " << statAfter;
      if (blockNeighbor) {
        // if the neighbor is blocked, all pkts are dropped
        return statAfter - statBefore == 0;
      } else {
        // counts ttl >= 128 packet only
        return statAfter - statBefore == getIpToMacAndClassID().size();
      }
    };
    WITH_RETRIES({ EXPECT_EVENTUALLY_TRUE(aclStatsMatch()); });
  }

  void classIDAfterNeighborResolveHelper(bool blockNeighbor) {
    /*
     * Resolve neighbors, then apply classID
     * Prod will typically follow this sequence as LookupClassUpdater is
     * implemented as a state observer which would update resolved neighbors
     * with classIDs.
     */
    applyNewState([this](std::shared_ptr<SwitchState> /*in*/) {
      auto state1 = this->addNeighbors(this->getProgrammedState());
      auto state2 = this->resolveNeighbors(state1);
      return state2;
    });

    if (blockNeighbor) {
      setMacAddrsToBlock();
    }
    applyNewState([this, blockNeighbor](std::shared_ptr<SwitchState> /*in*/) {
      auto state =
          this->updateClassID(this->getProgrammedState(), blockNeighbor);
      return state;
    });
  }

  void classIDWithResolveHelper(bool blockNeighbor) {
    if (blockNeighbor) {
      setMacAddrsToBlock();
    }
    applyNewState([this, blockNeighbor](std::shared_ptr<SwitchState> /*in*/) {
      auto state1 = this->addNeighbors(this->getProgrammedState());
      auto state2 = this->resolveNeighbors(state1);
      auto state3 = this->updateClassID(state2, blockNeighbor);
      return state3;
    });
  }

  void verifyHostToQueueMappingClassIDsAfterResolveHelper(bool blockNeighbor) {
    auto setup = [this, blockNeighbor]() {
      this->classIDAfterNeighborResolveHelper(blockNeighbor);
    };
    auto verify = [this, blockNeighbor]() {
      this->_verifyHelper(false, blockNeighbor);
      this->_verifyHelper(true, blockNeighbor);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void verifyHostToQueueMappingClassIDsWithResolveHelper(bool blockNeighbor) {
    auto setup = [this, blockNeighbor]() {
      this->classIDWithResolveHelper(blockNeighbor);
    };
    auto verify = [this, blockNeighbor]() {
      this->_verifyHelper(false, blockNeighbor);
      this->_verifyHelper(true, blockNeighbor);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  void verifyTtldCounter() {
    auto setup = [this]() {
      applyNewState([this](std::shared_ptr<SwitchState> /*in*/) {
        auto state1 = this->addNeighbors(this->getProgrammedState());
        auto state2 = this->resolveNeighbors(state1);
        return state2;
      });
    };

    auto verify = [this]() {
      auto ttlAclName = utility::getQueuePerHostTtlAclName();
      auto ttlCounterName = utility::getQueuePerHostTtlCounterName();

      for (bool frontPanel : {false, true}) {
        auto packetsBefore =
            utility::getAclInOutPackets(getSw(), ttlCounterName);

        auto bytesBefore =
            utility::getAclInOutPackets(getSw(), ttlCounterName, true);

        auto dstIP = getIpToMacAndClassID().begin()->first;
        sendPacket(dstIP, frontPanel, 64 /* ttl < 128 */);
        size_t packetSize = sendPacket(dstIP, frontPanel, 128 /* ttl >= 128 */);

        WITH_RETRIES({
          auto packetsAfter =
              utility::getAclInOutPackets(getSw(), ttlCounterName);

          auto bytesAfter =
              utility::getAclInOutPackets(getSw(), ttlCounterName, true);

          XLOG(DBG2) << "verify send packets "
                     << (frontPanel ? "out of port" : "switched") << "\n"
                     << "ttlAclPacketCounter: " << std::to_string(packetsBefore)
                     << " -> " << std::to_string(packetsAfter) << "\n"
                     << "ttlAclBytesCounter: " << std::to_string(bytesBefore)
                     << " -> " << std::to_string(bytesAfter);

          // counts ttl >= 128 packet only
          EXPECT_EVENTUALLY_EQ(packetsAfter - packetsBefore, 1);
          if (frontPanel) {
            EXPECT_EVENTUALLY_EQ(bytesAfter - bytesBefore, packetSize);
          }
          // TODO: Still need to debug why we get extra 4 bytes for CPU port
          EXPECT_EVENTUALLY_TRUE(bytesAfter - bytesBefore >= packetSize);
        });
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
    auto vlanId =
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
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
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), getSw()->needL2EntryForNeighbor());
      auto outPort = ecmpHelper.ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }

    return txPacketSize;
  }

  static inline constexpr auto kEcmpWidth = 1;
  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};
};

TYPED_TEST_SUITE(AgentQueuePerHostTest, TestTypes);

// Verify that traffic arriving on a front panel port gets right queue-per-host
// queue.
TYPED_TEST(
    AgentQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsAfterResolve) {
  this->verifyHostToQueueMappingClassIDsAfterResolveHelper(
      false /* block neighbor */);
}

// Verify that traffic arriving on a front panel port to a blocked neighbor gets
// dropped.
TYPED_TEST(
    AgentQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsAfterResolveBlock) {
  this->verifyHostToQueueMappingClassIDsAfterResolveHelper(
      true /* block neighbor */);
}

// Verify that traffic arriving on a front panel port gets right queue-per-host
// queue.
TYPED_TEST(AgentQueuePerHostTest, VerifyHostToQueueMappingClassIDsWithResolve) {
  this->verifyHostToQueueMappingClassIDsWithResolveHelper(
      false /* block neighbor */);
}

// Verify that traffic arriving on a front panel port to a blocked neighbor gets
// dropped.
TYPED_TEST(
    AgentQueuePerHostTest,
    VerifyHostToQueueMappingClassIDsWithResolveBlock) {
  this->verifyHostToQueueMappingClassIDsWithResolveHelper(
      true /* block neighbor */);
}

// Verify that TTLd traffic not going to queue-per-host has TTLd counter
// incremented.
TYPED_TEST(AgentQueuePerHostTest, VerifyTtldCounter) {
  this->verifyTtldCounter();
}

} // namespace facebook::fboss
