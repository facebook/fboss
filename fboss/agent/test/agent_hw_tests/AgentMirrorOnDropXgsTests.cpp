// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTestBase.h"

#include <gtest/gtest.h>

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/packet/bcm/XgsPsampMod.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

using namespace ::testing;

namespace {

// Parsed representation of a complete XGS MoD packet
struct XgsModPacketParsed {
  EthHdr ethHeader;
  IPv6Hdr ipv6Header;
  UDPHeader udpHeader;
  psamp::XgsPsampModPacket psampPacket;
};

// Deserialize a raw IOBuf containing a complete XGS MoD packet
XgsModPacketParsed deserializeXgsModPacket(const folly::IOBuf* buf) {
  XgsModPacketParsed parsed;
  folly::io::Cursor cursor(buf);
  parsed.ethHeader = EthHdr(cursor);
  parsed.ipv6Header = IPv6Hdr(cursor);
  parsed.udpHeader.parse(&cursor);
  parsed.psampPacket = psamp::XgsPsampModPacket::deserialize(cursor);
  return parsed;
}

// Expected values for verifying a captured XGS MoD packet.
// All fields are optional; only fields that are set will be checked.
struct XgsModPacketExpectedValues {
  // Outer Ethernet
  std::optional<folly::MacAddress> srcMac;
  std::optional<folly::MacAddress> dstMac;

  // Outer IPv6
  std::optional<folly::IPAddressV6> srcIp;
  std::optional<folly::IPAddressV6> dstIp;

  // Outer UDP
  std::optional<uint16_t> srcPort;
  std::optional<uint16_t> dstPort;

  // IPFIX / PSAMP
  std::optional<uint32_t> observationDomainId;
  std::optional<uint32_t> switchId;
  std::optional<uint16_t> ingressPort;
  std::optional<uint8_t> dropReasonIngress;
  std::optional<uint8_t> dropReasonMmu;

  // Inner (sampled) packet - Ethernet
  std::optional<folly::MacAddress> innerDstMac;
  std::optional<folly::MacAddress> innerSrcMac;

  // Inner (sampled) packet - IPv6
  std::optional<folly::IPAddressV6> innerSrcIp;
  std::optional<folly::IPAddressV6> innerDstIp;

  // Inner (sampled) packet - UDP
  std::optional<uint16_t> innerSrcPort;
  std::optional<uint16_t> innerDstPort;
};

// Verify a captured XGS MoD packet against expected values.
// Only checks fields that are set in the expected struct. Invariants
// (IPFIX version, PSAMP template ID, varLenIndicator, UDP checksum == 0,
// nextHeader == UDP) are always checked.
void verifyXgsModCapturedPacket(
    const XgsModPacketParsed& captured,
    const XgsModPacketExpectedValues& expected) {
  // Outer Ethernet
  if (expected.srcMac.has_value()) {
    EXPECT_EQ(captured.ethHeader.getSrcMac(), expected.srcMac.value());
  }
  if (expected.dstMac.has_value()) {
    EXPECT_EQ(captured.ethHeader.getDstMac(), expected.dstMac.value());
  }

  // Outer IPv6
  EXPECT_EQ(
      captured.ipv6Header.nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  if (expected.srcIp.has_value()) {
    EXPECT_EQ(captured.ipv6Header.srcAddr, expected.srcIp.value());
  }
  if (expected.dstIp.has_value()) {
    EXPECT_EQ(captured.ipv6Header.dstAddr, expected.dstIp.value());
  }

  // Outer UDP
  EXPECT_EQ(captured.udpHeader.csum, 0);
  if (expected.srcPort.has_value()) {
    EXPECT_EQ(captured.udpHeader.srcPort, expected.srcPort.value());
  }
  if (expected.dstPort.has_value()) {
    EXPECT_EQ(captured.udpHeader.dstPort, expected.dstPort.value());
  }

  // IPFIX header
  EXPECT_EQ(captured.psampPacket.ipfixHeader.version, psamp::IPFIX_VERSION);
  if (expected.observationDomainId.has_value()) {
    EXPECT_EQ(
        captured.psampPacket.ipfixHeader.observationDomainId,
        expected.observationDomainId.value());
  }

  // PSAMP template
  EXPECT_EQ(
      captured.psampPacket.templateHeader.templateId,
      psamp::XGS_PSAMP_TEMPLATE_ID);

  // PSAMP data
  EXPECT_EQ(
      captured.psampPacket.data.varLenIndicator,
      psamp::XGS_PSAMP_VAR_LEN_INDICATOR);
  if (expected.switchId.has_value()) {
    EXPECT_EQ(captured.psampPacket.data.switchId, expected.switchId.value());
  }
  if (expected.ingressPort.has_value()) {
    EXPECT_EQ(
        captured.psampPacket.data.ingressPort, expected.ingressPort.value());
  }
  if (expected.dropReasonIngress.has_value()) {
    EXPECT_EQ(
        captured.psampPacket.data.dropReasonIngress,
        expected.dropReasonIngress.value());
  }
  if (expected.dropReasonMmu.has_value()) {
    EXPECT_EQ(
        captured.psampPacket.data.dropReasonMmu,
        expected.dropReasonMmu.value());
  }

  // Inner (sampled) packet
  auto innerBuf = folly::IOBuf::copyBuffer(
      captured.psampPacket.data.sampledPacketData.data(),
      captured.psampPacket.data.sampledPacketData.size());
  folly::io::Cursor innerCursor(innerBuf.get());

  EthHdr innerEth(innerCursor);
  if (expected.innerDstMac.has_value()) {
    EXPECT_EQ(innerEth.getDstMac(), expected.innerDstMac.value());
  }
  if (expected.innerSrcMac.has_value()) {
    EXPECT_EQ(innerEth.getSrcMac(), expected.innerSrcMac.value());
  }

  IPv6Hdr innerIpv6(innerCursor);
  EXPECT_EQ(innerIpv6.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));
  if (expected.innerSrcIp.has_value()) {
    EXPECT_EQ(innerIpv6.srcAddr, expected.innerSrcIp.value());
  }
  if (expected.innerDstIp.has_value()) {
    EXPECT_EQ(innerIpv6.dstAddr, expected.innerDstIp.value());
  }

  UDPHeader innerUdp;
  innerUdp.parse(&innerCursor);
  if (expected.innerSrcPort.has_value()) {
    EXPECT_EQ(innerUdp.srcPort, expected.innerSrcPort.value());
  }
  if (expected.innerDstPort.has_value()) {
    EXPECT_EQ(innerUdp.dstPort, expected.innerDstPort.value());
  }
}

cfg::MirrorOnDropReport makeXgsModReport(
    const std::string& name,
    int16_t srcPort,
    const folly::IPAddressV6& collectorIp,
    int16_t dstPort,
    const folly::IPAddressV6& switchIp,
    std::optional<int32_t> samplingRate = std::nullopt) {
  cfg::MirrorOnDropReport report;
  report.name() = name;
  report.localSrcPort() = srcPort;
  report.collectorIp() = collectorIp.str();
  report.collectorPort() = dstPort;
  report.mtu() = 1500;
  report.dscp() = 0;
  if (samplingRate.has_value()) {
    report.samplingRate() = samplingRate.value();
  }
  report.mirrorPort().ensure().tunnel().ensure().srcIp() = switchIp.str();
  return report;
}

} // namespace

class AgentMirrorOnDropXgsTest : public AgentMirrorOnDropTestBase {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::MIRROR_ON_DROP,
        ProductionFeature::MIRROR_ON_DROP_XGS};
  }

 protected:
  void configureErspanMirror(
      cfg::SwitchConfig& config,
      const std::string& mirrorName,
      const folly::IPAddressV6& tunnelDstIp,
      const folly::IPAddressV6& tunnelSrcIp,
      const PortID& srcPortId) {
    cfg::Mirror mirror;
    mirror.name() = mirrorName;
    mirror.destination().ensure().tunnel().ensure().greTunnel().ensure().ip() =
        tunnelDstIp.str();
    mirror.destination()->tunnel()->srcIp() = tunnelSrcIp.str();
    mirror.truncate() = true;
    config.mirrors()->push_back(mirror);
    utility::findCfgPort(config, srcPortId)->ingressMirror() = mirrorName;
  }

  void waitForStatsToStabilize(const std::vector<PortID>& ports) {
    // Verify stats stay constant across 3 consecutive iterations
    // (4 stats calls total) to avoid flakiness from a single match.
    // WITH_RETRIES sleeps ~1s between iterations, providing the time gap.
    constexpr int kStableIterations = 3;
    int stableCount = 0;
    auto prevStats = getLatestPortStats(ports);
    WITH_RETRIES({
      auto curStats = getLatestPortStats(ports);
      if (std::all_of(ports.begin(), ports.end(), [&](const auto& portId) {
            return *curStats[portId].outUnicastPkts_() ==
                *prevStats[portId].outUnicastPkts_();
          })) {
        ++stableCount;
      } else {
        stableCount = 0;
      }
      prevStats = curStats;
      EXPECT_EVENTUALLY_GE(stableCount, kStableIterations);
    });
  }

  // Ingress buffer pool shared size large enough for MoD pool allocation
  // (2580 cells) plus PG min guarantees across all ports, but small enough
  // to trigger MMU drops without excessive packet injection.
  static constexpr auto kModGlobalSharedBytes{2'000'000};
};

// Verifies MoD captures packets dropped due to no matching route (default
// route discard) on Tomahawk5 (XGS platform).
TEST_F(AgentMirrorOnDropXgsTest, XgsModDefaultRouteDrop) {
  PortID injectionPortId = masterLogicalInterfacePortIds()[0];
  PortID mirrorPortId = masterLogicalInterfacePortIds()[1];
  PortID collectorPortId = masterLogicalInterfacePortIds()[2];
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  auto setup = [&]() {
    cfg::SwitchConfig config = getAgentEnsemble()->getCurrentConfig();

    config.mirrorOnDropReports()->push_back(makeXgsModReport(
        "xgs-mod-test",
        kMirrorSrcPort,
        kCollectorIp_,
        kMirrorDstPort,
        kSwitchIp_));

    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);

    // Wait for route to be fully installed and TamManager to resolve
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    // Setup packet snooper to capture MoD packets
    utility::SwSwitchPacketSnooper snooper(getSw(), "xgs-mod-snooper");
    snooper.ignoreUnclaimedRxPkts();

    // Send packet to trigger drop (no route for this destination)
    std::unique_ptr<TxPacket> pkt =
        sendPackets(1, injectionPortId, kDropDestIp);

    XLOG(INFO) << "Sent packet to trigger drop, waiting for MoD packet...";
    XLOG(INFO) << "Original packet:\n" << PktUtil::hexDump(pkt->buf());

    // Wait for and capture the MoD packet
    WITH_RETRIES_N(5, {
      XLOG(DBG3) << "Waiting for mirror packet...";
      std::optional<std::unique_ptr<folly::IOBuf>> frameRx =
          snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());

      if (frameRx.has_value()) {
        XLOG(INFO) << "Captured MoD packet:\n"
                   << PktUtil::hexDump(frameRx->get());

        auto parsed = deserializeXgsModPacket(frameRx->get());

        XgsModPacketExpectedValues expected{
            .srcMac = getLocalMacAddress(),
            .dstMac = kCollectorNextHopMac_,
            .srcIp = kSwitchIp_,
            .dstIp = kCollectorIp_,
            .srcPort = kMirrorSrcPort,
            .dstPort = kMirrorDstPort,
            .observationDomainId = 1,
            .switchId = 0,
            .ingressPort = static_cast<uint16_t>(injectionPortId),
            .dropReasonIngress = 0x1A, // L3 destination discard
            .dropReasonMmu = 0x00,
            .innerDstMac = getMacForFirstInterfaceWithPortsForTesting(
                getProgrammedState()),
            .innerSrcMac = utility::kLocalCpuMac(),
            .innerSrcIp = kPacketSrcIp_,
            .innerDstIp = kDropDestIp,
            .innerSrcPort = kPacketSrcPort,
            .innerDstPort = kPacketDstPort,
        };
        verifyXgsModCapturedPacket(parsed, expected);
      }
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Test verifies MoD captures MMU drops caused by buffer exhaustion.
TEST_F(AgentMirrorOnDropXgsTest, XgsModMmuDrop) {
  PortID injectionPortId = masterLogicalInterfacePortIds()[0];
  PortID collectorPortId = masterLogicalInterfacePortIds()[1];
  PortID txOffPortId = masterLogicalInterfacePortIds()[2];
  const int kPriority = 2;
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);
  XLOG(DBG3) << "Tx off port: " << portDesc(txOffPortId);

  auto setup = [&]() {
    cfg::SwitchConfig config = getAgentEnsemble()->getCurrentConfig();

    config.mirrorOnDropReports()->push_back(makeXgsModReport(
        "xgs-mod-mmu-drop-test",
        kMirrorSrcPort,
        kCollectorIp_,
        kMirrorDstPort,
        kSwitchIp_));

    // Lossy PG (headroom=0) with larger globalShared (kModGlobalSharedBytes)
    // to ensure enough shared buffer for the MoD pool (2580 cells) after PG
    // min guarantees, but small enough to trigger MMU drops quickly.
    auto hwAsic =
        checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
    utility::setupPfcBuffers(
        getAgentEnsemble(),
        config,
        {injectionPortId},
        {}, // losslessPgIds
        {kPriority}, // lossyPgIds
        {}, // tcToPgOverride
        utility::PfcBufferParams::getPfcBufferParams(
            hwAsic->getAsicType(), kModGlobalSharedBytes));

    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    applyNewConfig(config);
    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    setupEcmpTraffic(
        txOffPortId,
        kDropDestIp,
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState()));
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    // Disable TX on egress port to cause buffer buildup and MMU drops
    utility::setCreditWatchdogAndPortTx(getAgentEnsemble(), txOffPortId, false);

    utility::SwSwitchPacketSnooper snooper(getSw(), "xgs-mmu-drop-snooper");
    snooper.ignoreUnclaimedRxPkts();

    // Send traffic on the lossy PG priority to fill the buffer.
    // Drops occur once shared buffer is exhausted.
    XLOG(INFO) << "Sending packets to trigger MMU drops...";
    std::unique_ptr<TxPacket> pkt =
        sendPackets(10000, injectionPortId, kDropDestIp, kPriority);
    XLOG(INFO) << "Sample packet:\n" << PktUtil::hexDump(pkt->buf());

    WITH_RETRIES({
      auto portStats = getLatestPortStats(injectionPortId);
      XLOG(DBG2) << "In congestion discards: "
                 << *portStats.inCongestionDiscards_();
      EXPECT_EVENTUALLY_GT(*portStats.inCongestionDiscards_(), 0);
    });

    WITH_RETRIES_N(5, {
      XLOG(DBG3) << "Waiting for MoD packet...";
      std::optional<std::unique_ptr<folly::IOBuf>> frameRx =
          snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());

      if (frameRx.has_value()) {
        XLOG(INFO) << "Captured MoD packet for MMU drop:\n"
                   << PktUtil::hexDump(frameRx->get());

        auto parsed = deserializeXgsModPacket(frameRx->get());

        XgsModPacketExpectedValues expected{
            .srcMac = getLocalMacAddress(),
            .dstMac = kCollectorNextHopMac_,
            .srcIp = kSwitchIp_,
            .dstIp = kCollectorIp_,
            .srcPort = kMirrorSrcPort,
            .dstPort = kMirrorDstPort,
            .observationDomainId = 1,
            .switchId = 0,
            .ingressPort = static_cast<uint16_t>(injectionPortId),
            .dropReasonIngress = 0x00, // not an ingress pipeline drop
            .dropReasonMmu = 0x03, // egress port drop
            .innerSrcIp = kPacketSrcIp_,
            .innerDstIp = kDropDestIp,
            .innerSrcPort = kPacketSrcPort,
            .innerDstPort = kPacketDstPort,
        };
        verifyXgsModCapturedPacket(parsed, expected);
      }
    });

    // Note: Do NOT consume remaining MoD packets here.
    // Trapping MoD packets triggers more drops, causing infinite MoD loop.

    // Re-enable TX (required for SDK cleanup)
    utility::setCreditWatchdogAndPortTx(getAgentEnsemble(), txOffPortId, true);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Test verifies that MoD captures packets dropped by ACL rules.
TEST_F(AgentMirrorOnDropXgsTest, XgsModAclDrop) {
  PortID injectionPortId = masterLogicalInterfacePortIds()[0];
  PortID collectorPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  const folly::IPAddressV6 kAclDropDestIp{"2401:db00:e112:9100:1006::1"};

  auto setup = [&]() {
    cfg::SwitchConfig config = getAgentEnsemble()->getCurrentConfig();

    config.mirrorOnDropReports()->push_back(makeXgsModReport(
        "xgs-mod-acl-drop-test",
        kMirrorSrcPort,
        kCollectorIp_,
        kMirrorDstPort,
        kSwitchIp_));

    cfg::AclEntry aclEntry;
    aclEntry.name() = "acl-drop-by-dstip";
    aclEntry.dstIp() = fmt::format("{}/128", kAclDropDestIp.str());
    aclEntry.actionType() = cfg::AclActionType::DENY;
    utility::addAclEntry(&config, aclEntry, utility::kDefaultAclTable());

    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    applyNewConfig(config);
    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "xgs-mod-acl-drop-snooper");
    snooper.ignoreUnclaimedRxPkts();

    std::unique_ptr<TxPacket> pkt =
        sendPackets(1, injectionPortId, kAclDropDestIp);

    XLOG(INFO) << "Sent packet to trigger ACL drop, waiting for MoD packet...";
    XLOG(INFO) << "Original packet:\n" << PktUtil::hexDump(pkt->buf());

    WITH_RETRIES_N(5, {
      XLOG(DBG3) << "Waiting for mirror packet...";
      std::optional<std::unique_ptr<folly::IOBuf>> frameRx =
          snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());

      if (frameRx.has_value()) {
        XLOG(INFO) << "Captured MoD packet for ACL drop:\n"
                   << PktUtil::hexDump(frameRx->get());

        auto parsed = deserializeXgsModPacket(frameRx->get());

        XgsModPacketExpectedValues expected{
            .srcMac = getLocalMacAddress(),
            .dstMac = kCollectorNextHopMac_,
            .srcIp = kSwitchIp_,
            .dstIp = kCollectorIp_,
            .srcPort = kMirrorSrcPort,
            .dstPort = kMirrorDstPort,
            .observationDomainId = 1,
            .switchId = 0,
            .ingressPort = static_cast<uint16_t>(injectionPortId),
            .dropReasonIngress = 0x10, // ingress FP drop (ACL)
            .dropReasonMmu = 0x00,
            .innerDstMac = getMacForFirstInterfaceWithPortsForTesting(
                getProgrammedState()),
            .innerSrcMac = utility::kLocalCpuMac(),
            .innerSrcIp = kPacketSrcIp_,
            .innerDstIp = kAclDropDestIp,
            .innerSrcPort = kPacketSrcPort,
            .innerDstPort = kPacketDstPort,
        };
        verifyXgsModCapturedPacket(parsed, expected);
      }
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Test verifies that MoD sampling works correctly when samplingRate is
// configured. A large number of drops are generated by looping traffic on a
// port and mirroring it (via ERSPAN tunnel mirror) to a destination port
// where the mirrored copies are dropped by L2 dst MAC mismatch. MoD with
// sampling captures a subset of those drops.
//
// Flow:
// 1. Traffic loops on trafficPortId (MAC loopback + route + TTL decrement
//    disabled). Seed packets are injected and loop continuously.
// 2. An ERSPAN (GRE) tunnel mirror on trafficPortId encapsulates each
//    packet with outer dst IP = kMirrorTunnelDstIp and sends the copy to
//    mirrorEgressPortId (resolved from route by MirrorManager).
// 3. mirrorEgressPortId is in MAC loopback, so mirrored copies loop back
//    and re-enter the pipeline. The non-local dest MAC (kCollectorNonLocalMac)
//    populated in the ethernet packet results in drops on loopback.
// 4. MoD with sampling captures a fraction (1/kSamplingRate) of those drops.
// 5. MoD packets egress collectorPortId with a non-local MAC. When looped
//    back, they are dropped due to MAC mismatch, which itself may trigger
//    another sampled MoD packet. The low sampling rate (1/1000) ensures this
//    recursive process decays geometrically (subcritical branching).
// 6. The loop runs until enough drops accumulate (kMinDrops). Then the loop
//    is stopped by bringing the traffic port down/up.
// 7. Total drops and MoD packets are counted from port stats and compared.
//
// Counting dropped packets:
//   We use Tx counters (outUnicastPkts) on mirrorEgressPortId as the
//   primary drop count. Every packet that exits mirrorEgressPortId loops
//   back and is dropped (dst MAC mismatch). We also add inDiscards from
//   trafficPortId to capture any drops from looping packets after stopping
//   traffic with a port flap (experiments have yielded 0 for this counter,
//   but it is included defensively).
//
// Statistical model:
//   n = total dropped packets (outUnicastPkts on mirrorEgressPortId +
//       inDiscards on trafficPortId)
//   p = 1 / kSamplingRate
//   Each dropped packet is sampled with probability p. Each MoD packet is
//   itself dropped (MAC mismatch on collector port) and may recursively
//   trigger another MoD. For a single initial drop, the total number of
//   MoD packets it produces (across all recursive generations) follows a
//   geometric distribution with success probability (1-p). The total MoD
//   count is the sum of n independent geometric variables:
//     E[MoD] = n * p / (1 - p)
//     Var[MoD] = n * p / (1 - p)^2
//     sigma = sqrt(n * p) / (1 - p)
//   We use a 1% deviation threshold for the confidence bounds:
//     bounds = [0.99 * E[MoD], 1.01 * E[MoD]]
//   The coefficient of variation is sigma/mu = 1/sqrt(n*p). For the test
//   to produce a false failure, the observed MoD count must deviate by
//   more than 1% from the mean, i.e. |Z| > 0.01 * sqrt(n*p). With
//   n = 1B and p = 0.001, n*p = 1M, so the z-score threshold is
//   0.01 * sqrt(1M) = 10. The probability of |Z| > 10 under a normal
//   distribution is less than 1 in 10^23, so a false failure is
//   effectively impossible.
TEST_F(AgentMirrorOnDropXgsTest, XgsModWithSampling) {
  PortID trafficPortId = masterLogicalInterfacePortIds()[0];
  PortID mirrorEgressPortId = masterLogicalInterfacePortIds()[1];
  PortID collectorPortId = masterLogicalInterfacePortIds()[2];
  XLOG(DBG3) << "Traffic loop port: " << portDesc(trafficPortId);
  XLOG(DBG3) << "Mirror egress (drop) port: " << portDesc(mirrorEgressPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  const int kSamplingRate = 1000;
  const int64_t kMinDrops = 1'000'000'000;

  // Non-local MAC triggers L2 drops on loopback (dst MAC mismatch).
  const auto kCollectorNonLocalMac = folly::MacAddress::fromHBO(
      getMacForFirstInterfaceWithPortsForTesting(getProgrammedState())
          .u64HBO() +
      10);

  const folly::IPAddressV6 kTrafficLoopIp{
      "2401:7777:7777:7777:7777:7777:7777:7777"};
  const folly::IPAddressV6 kMirrorTunnelDstIp{
      "2401:6666:6666:6666:6666:6666:6666:6666"};
  const std::string kTrafficMirrorName = "traffic-mirror";

  auto setup = [&]() {
    cfg::SwitchConfig config = getAgentEnsemble()->getCurrentConfig();

    config.mirrorOnDropReports()->push_back(makeXgsModReport(
        "xgs-mod-sampling-test",
        kMirrorSrcPort,
        kCollectorIp_,
        kMirrorDstPort,
        kSwitchIp_,
        kSamplingRate));

    // No explicit egress port — MirrorManager resolves from route.
    configureErspanMirror(
        config,
        kTrafficMirrorName,
        kMirrorTunnelDstIp,
        kSwitchIp_,
        trafficPortId);

    applyNewConfig(config);

    setupEcmpTraffic(
        trafficPortId,
        kTrafficLoopIp,
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState()),
        true /*disableTtlDecrement*/);

    // Non-local dest MAC in the ethernet packet will result in drops on
    // loopback.
    setupEcmpTraffic(
        mirrorEgressPortId, kMirrorTunnelDstIp, kCollectorNonLocalMac);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNonLocalMac);

    waitForStateUpdates(getSw());

    WITH_RETRIES({
      auto mirrorState =
          getProgrammedState()->getMirrors()->getNodeIf(kTrafficMirrorName);
      ASSERT_EVENTUALLY_TRUE(mirrorState != nullptr);
      EXPECT_EVENTUALLY_TRUE(mirrorState->isResolved());
    });
  };

  auto verify = [&]() {
    auto state = getProgrammedState();
    auto mirrorOnDropReports = state->getMirrorOnDropReports();
    ASSERT_NE(mirrorOnDropReports, nullptr);
    auto report = mirrorOnDropReports->getNodeIf("xgs-mod-sampling-test");
    ASSERT_NE(report, nullptr);
    EXPECT_EQ(report->getSamplingRate(), kSamplingRate);

    const std::vector<PortID> allPorts = {
        trafficPortId, mirrorEgressPortId, collectorPortId};

    auto statsBefore = getLatestPortStats(allPorts);

    sendPackets(
        static_cast<int>(
            getAgentEnsemble()->getMinPktsForLineRate(trafficPortId)),
        trafficPortId,
        kTrafficLoopIp,
        0 /*priority*/,
        64 /*payloadSize*/);

    WITH_RETRIES_N(30, {
      auto mirrorEgressStats = getLatestPortStats(mirrorEgressPortId);
      int64_t mirrorOut = *mirrorEgressStats.outUnicastPkts_() -
          *statsBefore.at(mirrorEgressPortId).outUnicastPkts_();
      XLOG_EVERY_MS(INFO, 1000)
          << "Mirror egress outUnicastPkts so far: " << mirrorOut;
      EXPECT_EVENTUALLY_GE(mirrorOut, kMinDrops);
    });

    getAgentEnsemble()->getLinkToggler()->bringDownPorts({trafficPortId});
    getAgentEnsemble()->getLinkToggler()->bringUpPorts({trafficPortId});
    getAgentEnsemble()->waitForSpecificRateOnPort(trafficPortId, 0);
    getAgentEnsemble()->waitForSpecificRateOnPort(mirrorEgressPortId, 0);
    getAgentEnsemble()->waitForSpecificRateOnPort(collectorPortId, 0);

    waitForStatsToStabilize(allPorts);

    auto statsAfter = getLatestPortStats(allPorts);

    int64_t mirrorEgressDrops =
        *statsAfter[mirrorEgressPortId].outUnicastPkts_() -
        *statsBefore[mirrorEgressPortId].outUnicastPkts_();
    // Include inDiscards from trafficPort. Experiments have yielded 0 for
    // this counter, but we include it defensively in case any looping
    // packets are dropped after stopping traffic with a port flap.
    int64_t trafficPortDiscards = *statsAfter[trafficPortId].inDiscards_() -
        *statsBefore[trafficPortId].inDiscards_();
    int64_t droppedPackets = mirrorEgressDrops + trafficPortDiscards;
    int64_t modPacketsReceived =
        *statsAfter[collectorPortId].outUnicastPkts_() -
        *statsBefore[collectorPortId].outUnicastPkts_();

    XLOG(INFO) << "droppedPackets=" << droppedPackets
               << " (mirrorEgressDrops=" << mirrorEgressDrops
               << " trafficPortDiscards=" << trafficPortDiscards << ")"
               << " modPacketsReceived=" << modPacketsReceived;
    ASSERT_GT(droppedPackets, 0);
    ASSERT_GT(modPacketsReceived, 0);

    // inUnicastPkts is not populated on XGS MAC loopback; use bytes.
    int64_t trafficInBytes = *statsAfter[trafficPortId].inBytes_() -
        *statsBefore[trafficPortId].inBytes_();
    int64_t trafficOutBytes = *statsAfter[trafficPortId].outBytes_() -
        *statsBefore[trafficPortId].outBytes_();
    XLOG(INFO) << "trafficPort verification: inBytes=" << trafficInBytes
               << " outBytes=" << trafficOutBytes;
    EXPECT_GT(trafficInBytes, 0);
    EXPECT_GT(trafficOutBytes, 0);

    const double p = 1.0 / kSamplingRate;
    const double expectedModPackets = droppedPackets * p / (1.0 - p);
    const double deviation = 0.01 * expectedModPackets;
    const int64_t minExpected =
        static_cast<int64_t>(std::floor(expectedModPackets - deviation));
    const int64_t maxExpected =
        static_cast<int64_t>(std::ceil(expectedModPackets + deviation));

    XLOG(INFO) << "MoD sampling: received=" << modPacketsReceived
               << " expected=" << expectedModPackets << " bounds=["
               << minExpected << ", " << maxExpected << "]"
               << " drops=" << droppedPackets
               << " samplingRate=" << kSamplingRate;

    EXPECT_GE(modPacketsReceived, minExpected) << "Below expected range";
    EXPECT_LE(modPacketsReceived, maxExpected) << "Above expected range";
  };

  verifyAcrossWarmBoots(setup, verify);
}

// XGS warmboot test subclass. Config changes across warmboot require HW
// writes, so failHwCallsOnWarmboot must be false.
class AgentMirrorOnDropXgsWarmbootTest : public AgentMirrorOnDropXgsTest {
 protected:
  bool failHwCallsOnWarmboot() const override {
    return false;
  }
};

// Verifies warmboot enablement of MoD with sampling on TH5.
// Coldboot: switch comes up WITHOUT MoD. After warmboot, MoD with sampling
// is added and verified to be programmed on the ASIC by checking switch state.
TEST_F(AgentMirrorOnDropXgsWarmbootTest, XgsModWarmbootEnableSampling) {
  const int kSamplingRate = 90000;

  auto setup = []() {};

  auto verify = [&]() {
    auto state = getProgrammedState();
    auto reports = state->getMirrorOnDropReports();
    EXPECT_TRUE(reports == nullptr || reports->numNodes() == 0);
  };

  auto setupPostWb = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->push_back(makeXgsModReport(
        "xgs-mod-wb-enable-sampling",
        kMirrorSrcPort,
        kCollectorIp_,
        kMirrorDstPort,
        kSwitchIp_,
        kSamplingRate));
    applyNewConfig(config);
    waitForStateUpdates(getSw());
  };

  auto verifyPostWb = [&]() {
    auto state = getProgrammedState();
    auto reports = state->getMirrorOnDropReports();
    ASSERT_NE(reports, nullptr);
    auto report = reports->getNodeIf("xgs-mod-wb-enable-sampling");
    ASSERT_NE(report, nullptr);
    EXPECT_EQ(report->getSamplingRate(), kSamplingRate);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWb, verifyPostWb);
}

// Verifies warmboot disablement of MoD with sampling on TH5.
// Coldboot: switch comes up WITH MoD sampling configured and verified to be
// programmed on the ASIC by checking switch state. After warmboot, MoD config
// is removed and verified to be absent from switch state.
TEST_F(AgentMirrorOnDropXgsWarmbootTest, XgsModWarmbootDisableSampling) {
  const int kSamplingRate = 90000;

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->push_back(makeXgsModReport(
        "xgs-mod-wb-disable-sampling",
        kMirrorSrcPort,
        kCollectorIp_,
        kMirrorDstPort,
        kSwitchIp_,
        kSamplingRate));
    applyNewConfig(config);
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    auto state = getProgrammedState();
    auto reports = state->getMirrorOnDropReports();
    ASSERT_NE(reports, nullptr);
    auto report = reports->getNodeIf("xgs-mod-wb-disable-sampling");
    ASSERT_NE(report, nullptr);
    EXPECT_EQ(report->getSamplingRate(), kSamplingRate);
  };

  auto setupPostWb = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->clear();
    applyNewConfig(config);
    waitForStateUpdates(getSw());
  };

  auto verifyPostWb = [&]() {
    auto state = getProgrammedState();
    auto reports = state->getMirrorOnDropReports();
    EXPECT_TRUE(reports == nullptr || reports->numNodes() == 0);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWb, verifyPostWb);
}

} // namespace facebook::fboss
