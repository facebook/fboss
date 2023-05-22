/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/MacTableUtils.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestMacUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class HwQueuePerHostL2Test : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        getAsic()->desiredLoopbackMode());
    cfg.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;

    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addQueuePerHostQueueConfig(&cfg);
      utility::addQueuePerHostAcls(&cfg);
    }

    return cfg;
  }

  void verifyHelper(bool useFrontPanel) {
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

    auto txPacket = createL3Pkt(64 /* ttl < 128 */);
    auto txPacket2 = createL3Pkt(128 /* ttl >= 128 */);

    if (useFrontPanel) {
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket), PortID(masterLogicalPortIds()[1]));
      getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
          std::move(txPacket2), PortID(masterLogicalPortIds()[1]));
    } else {
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket2));
    }

    /*
     *  CPU originated packets:
     *     - Hits ACL (queue2Cnt = 1), egress through queue 2 of port0.
     *     - port0 is in loopback mode, so the packet gets looped back.
     *     - the looped back packet hits ACL (queue2Cnt = 2).
     *     - the packet gets dropped at egress as srcPort == dstPort, thus
     *       breaking the loop.
     *
     *  Front panel packets (injected with pipeline bypass):
     *     - Egress out of port1 queue0 (pipeline bypass).
     *     - port1 is in loopback mode, so the packet gets looped back.
     *     - Rest of the workflow is same as above when CPU originated packet
     *       gets injected for switching.
     *
     * Note: these are bridged packets, thus the looped back packets carry same
     * MAC as before and thus hit ACL.
     *
     * On some platforms, split horizon check is after ACL matching.
     */
    std::map<int, int64_t> afterQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      afterQueueOutPkts[queueId] =
          this->getLatestPortStats(this->masterLogicalPortIds()[0])
              .get_queueOutPackets_()
              .at(queueId);
    }

    for (auto [qid, beforePkts] : beforeQueueOutPkts) {
      auto pktsOnQueue = afterQueueOutPkts[qid] - beforePkts;

      XLOG(DBG0) << "queueId: " << qid << " pktsOnQueue: " << pktsOnQueue;

      if (qid == kQueueID()) {
        /*
         * On some platforms, split horizon check is after ACL matching.
         * Thus, the counter get increment one additional time for the looped
         * back packet.
         */
        if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO) {
          /* 1 pkt each for ttl < 128 and ttl >= 128 */
          EXPECT_EQ(pktsOnQueue, 4);
        } else {
          /* 1 pkt each for ttl < 128 and ttl >= 128 */
          EXPECT_EQ(pktsOnQueue, 2);
        }
      } else {
        EXPECT_EQ(pktsOnQueue, 0);
      }
    }

    auto statAfter = utility::getAclInOutPackets(
        getHwSwitch(), this->getProgrammedState(), ttlAclName, ttlCounterName);

    /*
     * counts ttl >= 128 packet only
     * but L2 traffic (so TTL is not decremented, and thus looped back packet
     * also has ttl >= 128, thus the count is 2.
     */
    EXPECT_EQ(statAfter - statBefore, 2);
  }

  std::unique_ptr<facebook::fboss::TxPacket> createL3Pkt(uint8_t ttl) {
    return utility::makeUDPTxPacket(
        getHwSwitch(),
        VlanID(*initialConfig().vlanPorts()[0].vlanID()),
        kMac1(),
        kMac0(), // dstMac: packet to port0 (from CPU/port1)
        folly::IPAddressV6("1::1"), // srcIPv6
        folly::IPAddressV6("1::10"), // dstIPv6
        8000, // l4 src port
        8001, // l4 dst port
        48 << 2, // DSCP
        ttl);
  }

  PortDescriptor physPortDescr() const {
    return PortDescriptor(PortID(masterLogicalPortIds()[0]));
  }

  VlanID kVlanID() {
    return VlanID(*initialConfig().vlanPorts()[0].vlanID());
  }

  int kQueueID() {
    return 2;
  }

  static MacAddress kMac0() {
    return MacAddress("02:00:00:00:00:05");
  }

  PortDescriptor kPhysPortDescr0() const {
    return PortDescriptor(PortID(masterLogicalPortIds()[0]));
  }

  static MacAddress kMac1() {
    return MacAddress("02:00:00:00:00:06");
  }

  PortDescriptor kPhysPortDescr1() const {
    return PortDescriptor(PortID(masterLogicalPortIds()[1]));
  }

  cfg::AclLookupClass kClassID1() {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1;
  }

  cfg::AclLookupClass kClassID0() {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }

  void addOrUpdateMacEntry(
      folly::MacAddress macAddr,
      PortDescriptor portDescr,
      std::optional<cfg::AclLookupClass> classID,
      MacEntryType type) {
    auto newState = getProgrammedState()->clone();
    auto vlan = newState->getMultiSwitchVlans()->getNodeIf(kVlanID()).get();
    auto macTable = vlan->getMacTable().get();
    macTable = macTable->modify(&vlan, &newState);
    if (macTable->getMacIf(macAddr)) {
      macTable->updateEntry(macAddr, portDescr, classID, type);
    } else {
      auto macEntry =
          std::make_shared<MacEntry>(macAddr, portDescr, classID, type);
      macTable->addEntry(macEntry);
    }
    applyNewState(newState);
  }
};

TEST_F(HwQueuePerHostL2Test, VerifyHostToQueueMappingClassIDCpu) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [this]() {
    addOrUpdateMacEntry(
        kMac0(), kPhysPortDescr0(), kClassID0(), MacEntryType::STATIC_ENTRY);
  };

  auto verify = [this]() { verifyHelper(false /* cpu port */); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwQueuePerHostL2Test, VerifyHostToQueueMappingClassIDFrontPanel) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
    return;
  }

  auto setup = [this]() {
    addOrUpdateMacEntry(
        kMac0(), kPhysPortDescr0(), kClassID0(), MacEntryType::STATIC_ENTRY);
    addOrUpdateMacEntry(
        kMac1(), kPhysPortDescr1(), kClassID1(), MacEntryType::STATIC_ENTRY);
  };

  auto verify = [this]() { verifyHelper(true /* front panel port */); };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
