// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/gen-cpp2/production_features_types.h"
#include "fboss/lib/CommonUtils.h"

#ifndef IS_OSS
#include <folly/logging/LoggerDB.h>
#include <folly/logging/test/TestLogHandler.h>
#endif

namespace facebook::fboss {

// Drop cause bit positions from the Chenab SDK telemetry pipeline.
// These map to sx_tele_drop_cause_*_reason_e enums in sx_tele_auto.h.
enum PipelineIpDropBit : int {
  kDipIsLoopback = 3,
};

enum PipelineMacDropBit : int {
  kSmacIsMulticast = 11,
};

enum IngressMacDropBit : int {
  kTypeOutOfRange = 4,
};

enum EgressMacDropBit : int {
  kEgressMtuError = 0,
};

class AgentDropBitmapTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::CUSTOM_DROP_BITMAP_SUPPORT};
  }

 protected:
  void sendRawEthPacket(
      folly::MacAddress srcMac,
      folly::MacAddress dstMac,
      ETHERTYPE etherType) {
    auto vlanId = getVlanIDForTx();
    auto pkt =
        utility::makeEthTxPacket(getSw(), vlanId, srcMac, dstMac, etherType);
    getSw()->sendPacketOutOfPortAsync(
        std::move(pkt), PortID(masterLogicalInterfaceOrHyperPortIds()[0]));
  }

  template <typename AddrT>
  void sendPacket(AddrT dstIp) {
    auto vlanId = getVlanIDForTx();
    auto intfMac = getMacForFirstInterfaceWithPorts(getProgrammedState());
    auto srcIp = AddrT(
        std::is_same_v<AddrT, folly::IPAddressV4> ? "10.0.0.1" : "1001::1");
    auto pkt = utility::makeUDPTxPacket(
        getSw(), vlanId, intfMac, intfMac, srcIp, dstIp, 10000, 10001, 0, 64);
    getSw()->sendPacketOutOfPortAsync(
        std::move(pkt), PortID(masterLogicalInterfaceOrHyperPortIds()[0]));
  }

  // OR-accumulate src into dst. Used to aggregate across switch indices
  // and across retry iterations (bitmaps are READ_AND_CLEAR, so drops
  // observed in earlier reads must be preserved).
  static void orBitmapStats(
      HwSwitchDropBitmapStats& dst,
      const HwSwitchDropBitmapStats& src) {
#define OR_BITMAP(field) \
  dst.field() = dst.field().value_or(0) | src.field().value_or(0);

    OR_BITMAP(ingressMacDropBitmap);
    OR_BITMAP(egressPipeDropBitmap);
    OR_BITMAP(egressMacDropBitmap);
    OR_BITMAP(pipelineGeneralDropBitmap);
    OR_BITMAP(pipelineMacDropBitmap);
    OR_BITMAP(pipelineMplsDropBitmap);
    OR_BITMAP(pipelineTunnelDropBitmap);
    OR_BITMAP(pipelineIpv4DropBitmap);
    OR_BITMAP(pipelineIpv6DropBitmap);
    OR_BITMAP(bufferDropBitmap);
    OR_BITMAP(queueDropBitmap);
    OR_BITMAP(hostDropBitmap);
#undef OR_BITMAP
  }

  HwSwitchDropBitmapStats getAggregatedSwitchDropBitmapStats() {
    HwSwitchDropBitmapStats result;
    checkWithRetry([&result, this]() {
      auto switchIndex2Stats = getAllHwSwitchStats();
      for (const auto& switchIndex :
           getSw()->getSwitchInfoTable().getSwitchIndices()) {
        auto it = switchIndex2Stats.find(switchIndex);
        if (it == switchIndex2Stats.end()) {
          return false;
        }
        // OR into the persistent result (not a per-iteration local): a
        // concurrent READ_AND_CLEAR collection may consume a switch's drops
        // between retries, so a partial snapshot seen on an earlier retry
        // must not be discarded or those drops are lost permanently.
        orBitmapStats(result, *it->second.switchDropBitmapStats());
      }
      return true;
    });
    return result;
  }

  enum class DropBitmapField {
    INGRESS_MAC,
    EGRESS_PIPE,
    EGRESS_MAC,
    PIPELINE_GENERAL,
    PIPELINE_MAC,
    PIPELINE_MPLS,
    PIPELINE_TUNNEL,
    PIPELINE_IPV4,
    PIPELINE_IPV6,
    BUFFER,
    QUEUE,
    HOST,
  };

  // Verify that only the expected drop reason fires and no other bitmap
  // categories report unexpected drops. This catches regressions where a
  // code or config change causes collateral drops in unrelated stages.
  void verifyOnlyExpectedDrop(
      const HwSwitchDropBitmapStats& stats,
      DropBitmapField expectedField,
      int expectedBit,
      const char* desc) {
    auto getBitmap = [&](DropBitmapField f) -> int64_t {
      switch (f) {
        case DropBitmapField::INGRESS_MAC:
          return stats.ingressMacDropBitmap().value_or(0);
        case DropBitmapField::EGRESS_PIPE:
          return stats.egressPipeDropBitmap().value_or(0);
        case DropBitmapField::EGRESS_MAC:
          return stats.egressMacDropBitmap().value_or(0);
        case DropBitmapField::PIPELINE_GENERAL:
          return stats.pipelineGeneralDropBitmap().value_or(0);
        case DropBitmapField::PIPELINE_MAC:
          return stats.pipelineMacDropBitmap().value_or(0);
        case DropBitmapField::PIPELINE_MPLS:
          return stats.pipelineMplsDropBitmap().value_or(0);
        case DropBitmapField::PIPELINE_TUNNEL:
          return stats.pipelineTunnelDropBitmap().value_or(0);
        case DropBitmapField::PIPELINE_IPV4:
          return stats.pipelineIpv4DropBitmap().value_or(0);
        case DropBitmapField::PIPELINE_IPV6:
          return stats.pipelineIpv6DropBitmap().value_or(0);
        case DropBitmapField::BUFFER:
          return stats.bufferDropBitmap().value_or(0);
        case DropBitmapField::QUEUE:
          return stats.queueDropBitmap().value_or(0);
        case DropBitmapField::HOST:
          return stats.hostDropBitmap().value_or(0);
      }
    };

    auto expected = getBitmap(expectedField);
    EXPECT_EQ(expected, 1LL << expectedBit)
        << desc << ": expected only bit " << expectedBit
        << " set in field, got: 0x" << std::hex << expected;

    for (auto f :
         {DropBitmapField::INGRESS_MAC,
          DropBitmapField::EGRESS_PIPE,
          DropBitmapField::EGRESS_MAC,
          DropBitmapField::PIPELINE_GENERAL,
          DropBitmapField::PIPELINE_MAC,
          DropBitmapField::PIPELINE_MPLS,
          DropBitmapField::PIPELINE_TUNNEL,
          DropBitmapField::PIPELINE_IPV4,
          DropBitmapField::PIPELINE_IPV6,
          DropBitmapField::BUFFER,
          DropBitmapField::QUEUE,
          DropBitmapField::HOST}) {
      if (f == expectedField) {
        continue;
      }
      EXPECT_EQ(getBitmap(f), 0)
          << desc << ": unexpected drop in other bitmap field, got: 0x"
          << std::hex << getBitmap(f);
    }
  }

  // Must be called in verify(), not setup(). The process restarts on
  // warm boot, destroying any handler installed during setup(). Installing
  // in verify() ensures log capture works on both CB and WB iterations.
#ifndef IS_OSS
  std::shared_ptr<folly::TestLogHandler> installLogHandler() {
    auto handler = std::make_shared<folly::TestLogHandler>();
    folly::LoggerDB::get().getCategory("")->addHandler(handler);
    return handler;
  }

  // Verify that the drop reason logging pipeline is working end-to-end:
  // the bitmap was collected, decoded, and logged by logDropBitmapReasons.
  void verifyDropReasonLogged(
      const std::shared_ptr<folly::TestLogHandler>& handler,
      const std::string& expectedSubstring,
      const char* desc) {
    bool found = false;
    for (const auto& [msg, cat] : handler->getMessages()) {
      if (msg.getMessage().find(expectedSubstring) != std::string::npos) {
        found = true;
        break;
      }
    }
    EXPECT_TRUE(found) << desc << ": expected log containing '"
                       << expectedSubstring << "' not found";
  }
#else
  // folly/logging/test (TestLogHandler) is not shipped by the OSS folly
  // build, so log capture is unavailable there. Provide no-op shims so the
  // call sites are identical across builds; the bitmap stat assertions
  // (verifyOnlyExpectedDrop) still run and validate drop detection in OSS.
  std::shared_ptr<void> installLogHandler() {
    return nullptr;
  }
  void verifyDropReasonLogged(
      const std::shared_ptr<void>& /*handler*/,
      const std::string& /*expectedSubstring*/,
      const char* /*desc*/) {}
#endif
};

TEST_F(AgentDropBitmapTest, pipelineIpv4Ipv6Drops) {
  auto verify = [&]() {
    // Installed in verify() to capture logs during both CB and WB iterations.
    auto logHandler = installLogHandler();

    // IPv4 loopback DIP drop
    sendPacket(folly::IPAddressV4("127.0.0.1"));
    HwSwitchDropBitmapStats ipv4Stats;
    WITH_RETRIES({
      orBitmapStats(ipv4Stats, getAggregatedSwitchDropBitmapStats());
      EXPECT_EVENTUALLY_TRUE(
          ipv4Stats.pipelineIpv4DropBitmap().value_or(0) &
          (1LL << PipelineIpDropBit::kDipIsLoopback));
    });
    verifyOnlyExpectedDrop(
        ipv4Stats,
        DropBitmapField::PIPELINE_IPV4,
        PipelineIpDropBit::kDipIsLoopback,
        "IPv4 loopback DIP");
    verifyDropReasonLogged(
        logHandler, "DROP reasons pipelineIpv4", "IPv4 loopback DIP log");

    // Drain residual IPv4 stats before starting IPv6 phase.
    // Bitmaps are READ_AND_CLEAR — wait for the next collection
    // cycle to clear the IPv4 drop so it doesn't leak into IPv6.
    WITH_RETRIES({
      auto drain = getAggregatedSwitchDropBitmapStats();
      EXPECT_EVENTUALLY_EQ(drain.pipelineIpv4DropBitmap().value_or(0), 0);
    });

    // IPv6 loopback DIP drop
    sendPacket(folly::IPAddressV6("::1"));
    HwSwitchDropBitmapStats ipv6Stats;
    WITH_RETRIES({
      orBitmapStats(ipv6Stats, getAggregatedSwitchDropBitmapStats());
      EXPECT_EVENTUALLY_TRUE(
          ipv6Stats.pipelineIpv6DropBitmap().value_or(0) &
          (1LL << PipelineIpDropBit::kDipIsLoopback));
    });
    verifyOnlyExpectedDrop(
        ipv6Stats,
        DropBitmapField::PIPELINE_IPV6,
        PipelineIpDropBit::kDipIsLoopback,
        "IPv6 loopback DIP");
    verifyDropReasonLogged(
        logHandler, "DROP reasons pipelineIpv6", "IPv6 loopback DIP log");
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentDropBitmapTest, pipelineMacAndIngressMacDrops) {
  auto verify = [&]() {
    // Installed in verify() to capture logs during both CB and WB iterations.
    auto logHandler = installLogHandler();
    auto intfMac = getMacForFirstInterfaceWithPorts(getProgrammedState());

    // Pipeline MAC: SMAC multicast drop
    sendRawEthPacket(
        folly::MacAddress("01:00:5e:aa:aa:aa"),
        intfMac,
        ETHERTYPE::ETHERTYPE_IPV4);
    HwSwitchDropBitmapStats macStats;
    WITH_RETRIES({
      orBitmapStats(macStats, getAggregatedSwitchDropBitmapStats());
      EXPECT_EVENTUALLY_TRUE(
          macStats.pipelineMacDropBitmap().value_or(0) &
          (1LL << PipelineMacDropBit::kSmacIsMulticast));
    });
    verifyOnlyExpectedDrop(
        macStats,
        DropBitmapField::PIPELINE_MAC,
        PipelineMacDropBit::kSmacIsMulticast,
        "Pipeline MAC SMAC multicast");
    verifyDropReasonLogged(
        logHandler, "DROP reasons pipelineMac", "Pipeline MAC SMAC log");

    // Drain residual pipelineMac stats before ingress MAC phase.
    WITH_RETRIES({
      auto drain = getAggregatedSwitchDropBitmapStats();
      EXPECT_EVENTUALLY_EQ(drain.pipelineMacDropBitmap().value_or(0), 0);
    });

    // Ingress MAC: type out of range drop
    // EtherType 0x05E0 is in the reserved range 0x05DD..0x05FF
    // This also triggers PIPELINE_MAC_INVALID_ETHERTYPE — the packet
    // traverses multiple pipeline stages that each flag it independently.
    // We verify the primary drop (ingressMac) without cross-contamination
    // checks since multi-stage drops are expected.
    sendRawEthPacket(
        folly::MacAddress("00:00:00:00:00:01"),
        intfMac,
        static_cast<ETHERTYPE>(0x05E0));
    HwSwitchDropBitmapStats ingressMacStats;
    WITH_RETRIES({
      orBitmapStats(ingressMacStats, getAggregatedSwitchDropBitmapStats());
      EXPECT_EVENTUALLY_TRUE(
          ingressMacStats.ingressMacDropBitmap().value_or(0) &
          (1LL << IngressMacDropBit::kTypeOutOfRange));
    });
    verifyDropReasonLogged(
        logHandler, "DROP reasons ingressMac", "Ingress MAC type OOR log");
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentDropBitmapTest, egressMacMtuDrop) {
  auto egressPort = PortDescriptor(masterLogicalInterfaceOrHyperPortIds()[0]);
  auto injectionPort = masterLogicalInterfaceOrHyperPortIds()[1];

  auto setup = [&]() {
    auto config = initialConfig(*getAgentEnsemble());
    for (auto& port : *config.ports()) {
      if (PortID(*port.logicalID()) == egressPort.phyPortID()) {
        port.maxFrameSize() = 200;
      }
    }
    applyNewConfig(config);

    utility::EcmpSetupTargetedPorts6 helper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());

    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return helper.resolveNextHops(in, {egressPort});
    });

    auto wrapper = getSw()->getRouteUpdater();
    using Prefix = typename Route<folly::IPAddressV6>::Prefix;
    helper.programRoutes(
        &wrapper, {egressPort}, {Prefix{folly::IPAddressV6("2001::1"), 128}});
  };

  auto verify = [&]() {
    // Installed in verify() to capture logs during both CB and WB iterations.
    auto logHandler = installLogHandler();
    auto intfMac = getMacForFirstInterfaceWithPorts(getProgrammedState());

    auto pkt = utility::makeUDPTxPacket(
        getSw(),
        getVlanIDForTx(),
        intfMac,
        intfMac,
        folly::IPAddressV6("1001::1"),
        folly::IPAddressV6("2001::1"),
        10000,
        10001,
        0,
        64,
        std::vector<uint8_t>(1400, 0xff));
    getSw()->sendPacketOutOfPortAsync(std::move(pkt), PortID(injectionPort));

    // No verifyOnlyExpectedDrop here: unlike the early-stage pipeline drops,
    // this is a well-formed routed packet that traverses ingress, pipeline
    // and routing before being dropped at the egress MAC MTU check, so other
    // stage bitmaps may legitimately be set. We assert only the egress MAC
    // MTU bit (same rationale as the ingress MAC phase).
    HwSwitchDropBitmapStats stats;
    WITH_RETRIES({
      orBitmapStats(stats, getAggregatedSwitchDropBitmapStats());
      EXPECT_EVENTUALLY_TRUE(
          stats.egressMacDropBitmap().value_or(0) &
          (1LL << EgressMacDropBit::kEgressMtuError));
    });
    verifyDropReasonLogged(
        logHandler, "DROP reasons egressMac", "Egress MTU log");
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
