/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"

#include <memory>

namespace facebook::fboss {

class AgentDot1qMappingTest : public AgentHwTest {
 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L2_QOS};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_strip_vlan_for_pipeline_bypass = false;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto switchId = ensemble.getSw()
                        ->getScopeResolver()
                        ->scope(ensemble.masterLogicalPortIds())
                        .switchId();
    auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(switchId);
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1],
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    cfg.switchSettings()->l2LearningMode() = cfg::L2LearningMode::DISABLED;

    // Enable VLAN tagging (emitTags) for all ports so they send/receive tagged
    // packets
    for (auto& vlanPort : *cfg.vlanPorts()) {
      vlanPort.emitTags() = true;
    }

    // Add Olympic QoS maps with PCP to TC and TC to queue mapping
    auto l3Asics = ensemble.getL3Asics();
    addOlympicPcpQosMaps(cfg, l3Asics);
    return cfg;
  }

  std::map<int, std::vector<uint8_t>> getOlympicQueueToPcp() const {
    // Map Olympic queue IDs to PCP values
    // This follows the same structure as kOlympicQueueToDscp in
    // OlympicTestUtils
    return {
        {utility::kOlympicSilverQueueId, {0, 2}},
        {utility::kOlympicGoldQueueId, {3}},
        {utility::kOlympicEcn1QueueId, {1}},
        {utility::kOlympicBronzeQueueId, {4, 5}},
        {utility::kOlympicICPQueueId, {6}},
        {utility::kOlympicNCQueueId, {7}}};
  }

  void addOlympicPcpQosMaps(
      cfg::SwitchConfig& cfg,
      const std::vector<const HwAsic*>& asics) const {
    // Add Olympic QoS configuration with bidirectional PCP mappings:
    // - DOT1P_TO_TC: Ingress PCP -> Traffic Class
    // - TC_AND_COLOR_TO_DOT1P: Traffic Class -> Egress PCP (for remarking)
    // Both mappings are created together when PCP map is present in QoS policy
    utility::addOlympicQosMapsForDot1p(cfg, getOlympicQueueToPcp(), asics);
  }

  std::map<int, int> getOlympicPcpToQueue() const {
    // Map PCP values to Olympic queue IDs that match getOlympicQueueToPcp()
    // This must align with the actual QoS configuration applied to the switch
    return {
        {0, utility::kOlympicSilverQueueId}, // PCP 0 -> Queue 0 (Silver)
        {1, utility::kOlympicEcn1QueueId}, // PCP 1 -> Queue 2 (ECN1)
        {2, utility::kOlympicSilverQueueId}, // PCP 2 -> Queue 0 (Silver)
        {3, utility::kOlympicGoldQueueId}, // PCP 3 -> Queue 1 (Gold)
        {4, utility::kOlympicBronzeQueueId}, // PCP 4 -> Queue 4 (Bronze)
        {5, utility::kOlympicBronzeQueueId}, // PCP 5 -> Queue 4 (Bronze)
        {6, utility::kOlympicICPQueueId}, // PCP 6 -> Queue 6 (ICP)
        {7,
         utility::kOlympicNCQueueId} // PCP 7 -> Queue 7 (NC - highest priority)
    };
  }

  static MacAddress kSourceMac() {
    return MacAddress("02:00:00:00:00:05");
  }

  void addStaticMacToConfig(
      cfg::SwitchConfig& cfg,
      const folly::MacAddress& mac,
      VlanID vlanId,
      PortID portId) {
    cfg::StaticMacEntry staticMac;
    staticMac.macAddress() = mac.toString();
    staticMac.vlanID() = static_cast<int32_t>(vlanId);
    staticMac.egressLogicalPortID() = static_cast<int32_t>(portId);

    if (!cfg.staticMacAddrs().has_value()) {
      cfg.staticMacAddrs() = std::vector<cfg::StaticMacEntry>();
    }
    cfg.staticMacAddrs()->push_back(staticMac);
  }

  void verifyStaticMacEntry(
      const folly::MacAddress& mac,
      VlanID vlanId,
      const PortID& portId,
      bool shouldExist) {
    auto state = getProgrammedState();
    auto vlan = state->getVlans()->getNodeIf(vlanId);
    EXPECT_TRUE(vlan) << "VLAN " << vlanId << " not found";

    auto macTable = vlan->getMacTable();
    auto macEntry = macTable->getMacIf(mac);

    if (!shouldExist) {
      if (macEntry) {
        EXPECT_NE(macEntry->getPort(), PortDescriptor(portId))
            << "MAC entry " << mac.toString() << " should not exist on port "
            << portId << " in VLAN " << vlanId
            << ", but found: " << macEntry->str();
      }
      return;
    }

    ASSERT_TRUE(macEntry) << "MAC entry " << mac.toString()
                          << " not found in VLAN " << vlanId;
    EXPECT_EQ(macEntry->getPort(), PortDescriptor(portId))
        << "MAC entry port mismatch";
    EXPECT_EQ(macEntry->getType(), MacEntryType::STATIC_ENTRY)
        << "MAC entry should be static";

    EXPECT_TRUE(macEntry->getConfigured().has_value())
        << "MAC entry should have configured field set";
    EXPECT_TRUE(macEntry->getConfigured().value())
        << "MAC entry should be marked as configured";
  }

  MacAddress kStaticMac1() {
    return MacAddress("02:00:00:00:01:01");
  }

  MacAddress kStaticMac2() {
    return MacAddress("02:00:00:00:01:02");
  }

  void sendPacketWithPcp(uint8_t pcp, PortID ingressPort) {
    auto vlanId =
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID());

    // Send 10 packets with the specified PCP value
    for (int i = 0; i < 10; i++) {
      // Create tagged Ethernet packet with PCP value
      auto txPacket = utility::makeEthTxPacket(
          getSw(),
          vlanId,
          kSourceMac(),
          kStaticMac2(),
          ETHERTYPE::ETHERTYPE_LLDP,
          std::vector<uint8_t>(64, 0xff));

      // Modify VLAN header to set PCP value
      // VLAN tag is at offset 12-15 (after dst MAC 6 bytes + src MAC 6 bytes)
      // Format: TPID (0x8100) | TCI (PCP:3bits, DEI:1bit, VID:12bits)
      auto buf = txPacket->buf();
      folly::io::RWPrivateCursor rwCursor(buf);

      // Skip dst MAC (6 bytes) and src MAC (6 bytes)
      rwCursor.skip(12);

      // Read and skip TPID (should be 0x8100)
      rwCursor.skip(2);

      // Construct new TCI with desired PCP
      uint16_t tci =
          (static_cast<uint16_t>(pcp & 0x7) << 13) | // PCP in top 3 bits
          (0 << 12) | // DEI in bit 12
          (static_cast<uint16_t>(vlanId) & 0xFFF); // VID in bottom 12 bits

      // Write new TCI value at position 14
      buf->writableData()[14] = (tci >> 8) & 0xFF;
      buf->writableData()[15] = tci & 0xFF;

      XLOG(DBG2) << "Sending packet " << i
                 << " with PCP=" << static_cast<int>(pcp) << " VLAN=" << vlanId
                 << " TCI=0x" << std::hex << tci << std::dec << " from port "
                 << ingressPort << " to " << kStaticMac2();

      // Send packet from ingress port
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), ingressPort);
    }
  }

  void verifyPcpQueueMapping(
      uint8_t pcp,
      int expectedQueueId,
      PortID ingressPort,
      PortID egressPort) {
    // Check queue OUT packets on egress port to see QoS classification
    auto beforeQueueOutPkts =
        folly::copy(getLatestPortStats(egressPort).queueOutPackets_().value());

    sendPacketWithPcp(pcp, ingressPort);

    WITH_RETRIES({
      auto afterQueueOutPkts = folly::copy(
          getLatestPortStats(egressPort).queueOutPackets_().value());

      XLOG(DBG2) << "verify send packets with PCP=" << static_cast<int>(pcp)
                 << " from ingress port " << ingressPort << " to egress port "
                 << egressPort << " expectedQueueId = " << expectedQueueId
                 << " (checking queue OUT packets on egress port " << egressPort
                 << ")";

      // Log all queue counters to see which queue is actually receiving packets
      for (int qid = 0; qid < afterQueueOutPkts.size(); qid++) {
        auto before = beforeQueueOutPkts.at(qid);
        auto after = afterQueueOutPkts.at(qid);
        if (after != before) {
          XLOG(DBG2) << "Egress port " << egressPort << " Queue " << qid
                     << " OUT packets: before=" << before << " after=" << after
                     << " delta=" << (after - before);
        }
      }

      auto expectedBefore = beforeQueueOutPkts.at(expectedQueueId);
      auto expectedAfter = afterQueueOutPkts.at(expectedQueueId);
      XLOG(DBG2) << "Egress port " << egressPort << " Expected queue "
                 << expectedQueueId << " OUT packets: before=" << expectedBefore
                 << " after=" << expectedAfter
                 << " delta=" << (expectedAfter - expectedBefore);

      EXPECT_EVENTUALLY_GE(expectedAfter - expectedBefore, 10);
    });
  }
};

TEST_F(AgentDot1qMappingTest, verifyDot1qMapping) {
  auto setup = [this]() {
    auto newCfg = initialConfig(*getAgentEnsemble());
    auto vlanId = VlanID(*newCfg.vlanPorts()[0].vlanID());
    auto portId1 = PortID(masterLogicalPortIds()[0]);
    auto portId2 = PortID(masterLogicalPortIds()[1]);

    // Add static MAC entries to config to enable L2 forwarding between ports
    addStaticMacToConfig(newCfg, kStaticMac1(), vlanId, portId1);
    addStaticMacToConfig(newCfg, kStaticMac2(), vlanId, portId2);

    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    auto vlanId =
        VlanID(*initialConfig(*getAgentEnsemble()).vlanPorts()[0].vlanID());
    auto portId1 = PortID(masterLogicalPortIds()[0]);
    auto portId2 = PortID(masterLogicalPortIds()[1]);

    // Verify static MAC entries exist in switch state
    verifyStaticMacEntry(kStaticMac1(), vlanId, portId1, true);
    verifyStaticMacEntry(kStaticMac2(), vlanId, portId2, true);

    // Verify PCP to queue mapping
    // Packets sent from port[0] to static MAC on port[1] should be queued
    // based on PCP value in the VLAN tag
    const auto& pcpToQueue = getOlympicPcpToQueue();

    // Test all PCP values (0-7) and verify they map to correct queues
    // Send from portId1, packets will egress on portId2 to kStaticMac2
    XLOG(DBG2) << "Testing PCP 0 (NCNF) -> Queue " << pcpToQueue.at(0);
    verifyPcpQueueMapping(0, pcpToQueue.at(0), portId1, portId2);

    XLOG(DBG2) << "Testing PCP 1 (ECN1) -> Queue " << pcpToQueue.at(1);
    verifyPcpQueueMapping(1, pcpToQueue.at(1), portId1, portId2);

    XLOG(DBG2) << "Testing PCP 2 (Silver) -> Queue " << pcpToQueue.at(2);
    verifyPcpQueueMapping(2, pcpToQueue.at(2), portId1, portId2);

    XLOG(DBG2) << "Testing PCP 3 (Gold) -> Queue " << pcpToQueue.at(3);
    verifyPcpQueueMapping(3, pcpToQueue.at(3), portId1, portId2);

    XLOG(DBG2) << "Testing PCP 4 (Bronze) -> Queue " << pcpToQueue.at(4);
    verifyPcpQueueMapping(4, pcpToQueue.at(4), portId1, portId2);

    XLOG(DBG2) << "Testing PCP 5 (Bronze) -> Queue " << pcpToQueue.at(5);
    verifyPcpQueueMapping(5, pcpToQueue.at(5), portId1, portId2);

    XLOG(DBG2) << "Testing PCP 6 (ICP) -> Queue " << pcpToQueue.at(6);
    verifyPcpQueueMapping(6, pcpToQueue.at(6), portId1, portId2);

    XLOG(DBG2) << "Testing PCP 7 (NC) -> Queue " << pcpToQueue.at(7);
    verifyPcpQueueMapping(7, pcpToQueue.at(7), portId1, portId2);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
