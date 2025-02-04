// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/MacAddress.h>
#include <gtest/gtest.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

// TODO(maxgg): remove once CS00012385223 is resolved
DEFINE_bool(
    mod_pp_test_use_pipeline_lookup,
    false,
    "Use pipeline lookup mode in packet processing drop test");

const std::string kSflowMirror = "sflow_mirror";
const std::string kModPacketAcl = "mod_packet_acl";
const std::string kModPacketAclCouter = "mod_packet_acl_counter";

namespace facebook::fboss {

using namespace ::testing;

// Flow of the test:
// 1. Setup mirror on drop, using a Recycle port as the MoD port.
// 2. Pick an Interface port and add a route to the MoD collector IP.
// 3. Trigger some packet drops.
// 4. A MoD packet will be generated and injected into the port picked in (1).
// 5. The MoD packet will be routed and sent out of the port picked in (2).
// 6. The MoD packet will be looped back and punted to the CPU by ACL.
// 7. We capture the packet using PacketSnooper and verify the contents.
class AgentMirrorOnDropTest
    : public AgentHwTest,
      public testing::WithParamInterface<cfg::PortType> {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return config;
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::MIRROR_ON_DROP};
  }

  void TearDown() override {
    // Log drop counters to facilitate test debugging.
    if (auto ensemble = getAgentEnsemble(); ensemble != nullptr) {
      std::string output;
      ensemble->runDiagCommand("diag count g\n", output);
      XLOG(DBG3) << output;
    }
    AgentHwTest::TearDown();
  }

 protected:
  // Packet to be dropped
  const folly::IPAddressV6 kDropDestIp{
      "2401:3333:3333:3333:3333:3333:3333:3333"};

  // MOD/collector addresses
  const folly::MacAddress kCollectorNextHopMac_{"02:88:88:88:88:88"};
  const folly::IPAddressV6 kCollectorIp_{
      "2401:9999:9999:9999:9999:9999:9999:9999"};
  const int16_t kMirrorSrcPort = 0x6666;
  const int16_t kMirrorDstPort = 0x7777;

  // MOD settings
  const int kTruncateSize = 128;
  const int kEventorPayloadSize = 96;

  // MOD constants.
  const int kRouteMissingTrapID = 0x1B;

  std::string portDesc(PortID portId) {
    const auto& cfg = getAgentEnsemble()->getCurrentConfig();
    for (const auto& port : *cfg.ports()) {
      if (PortID(*port.logicalID()) == portId) {
        return folly::sformat(
            "portId={} name={}", *port.logicalID(), *port.name());
      }
    }
    return "";
  }

  void setupEcmpTraffic(
      PortID portId,
      const folly::IPAddressV6& addr,
      const folly::MacAddress& nextHopMac) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(), nextHopMac};
    const PortDescriptor port{portId};
    RoutePrefixV6 route{addr, 128};
    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, {port});
    });
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&routeUpdater, {port}, {route});
  }

  cfg::MirrorOnDropEventConfig makeEventConfig(
      std::optional<cfg::MirrorOnDropAgingGroup> group,
      const std::vector<cfg::MirrorOnDropReasonAggregation>& reasonAggs) {
    cfg::MirrorOnDropEventConfig eventCfg;
    if (group.has_value()) {
      eventCfg.agingGroup() = group.value();
    }
    eventCfg.dropReasonAggregations() = reasonAggs;
    return eventCfg;
  }

  void setupMirrorOnDrop(
      cfg::SwitchConfig* config,
      PortID portId,
      const folly::IPAddressV6& collectorIp,
      const std::map<int8_t, cfg::MirrorOnDropEventConfig>&
          modEventToConfigMap) {
    cfg::MirrorOnDropReport report;
    report.name() = "mod-1";
    report.mirrorPortId() = portId;
    report.localSrcPort() = kMirrorSrcPort;
    report.collectorIp() = collectorIp.str();
    report.collectorPort() = kMirrorDstPort;
    report.mtu() = 1500;
    report.truncateSize() = kTruncateSize;
    report.dscp() = 0;
    report.modEventToConfigMap() = modEventToConfigMap;
    report.agingGroupAgingIntervalUsecs()[cfg::MirrorOnDropAgingGroup::GLOBAL] =
        100;
    report.agingGroupAgingIntervalUsecs()[cfg::MirrorOnDropAgingGroup::PORT] =
        200;
    config->mirrorOnDropReports()->push_back(report);

    // Also add an ACL for counting MOD packets
    auto* acl = utility::addAcl(config, kModPacketAcl);
    acl->dstIp() = kCollectorIp_.str();
    acl->l4DstPort() = kMirrorDstPort;
    utility::addAclStat(
        config,
        kModPacketAcl,
        kModPacketAclCouter,
        utility::getAclCounterTypes(getAgentEnsemble()->getL3Asics()));
  }

  void configureMirror(cfg::SwitchConfig& cfg) const {
    utility::configureSflowMirror(
        cfg,
        kSflowMirror,
        true /*truncate*/,
        utility::getSflowMirrorDestination(false /*v4*/).str());
  }

  template <typename T = folly::IPAddressV6>
  void resolveRouteForMirrorDestination(
      PortID mirrorDestinationPort,
      cfg::PortType portType) {
    boost::container::flat_set<PortDescriptor> nhopPorts{
        PortDescriptor(mirrorDestinationPort)};

    applyNewState(
        [&](const std::shared_ptr<SwitchState>& state) {
          utility::EcmpSetupTargetedPorts<T> ecmpHelper(
              state, RouterID(0), {portType});
          auto newState = ecmpHelper.resolveNextHops(state, nhopPorts);
          return newState;
        },
        "resolve mirror nexthop");

    auto mirror = getSw()->getState()->getMirrors()->getNodeIf(kSflowMirror);
    auto dip = mirror->getDestinationIp();

    RoutePrefix<T> prefix(T(dip->str()), dip->bitCount());
    utility::EcmpSetupTargetedPorts<T> ecmpHelper(
        getProgrammedState(), RouterID(0), {portType});

    ecmpHelper.programRoutes(
        getAgentEnsemble()->getRouteUpdaterWrapper(), nhopPorts, {prefix});
  }

  // Create a packet that will be dropped. The entire packet is expected to be
  // copied to the MoD payload. However note that if the test involves looping
  // traffic before they are dropped (e.g. by triggering buffer exhaustion), the
  // src/dst MAC defined here may be different from the MoD payload.
  std::unique_ptr<TxPacket> makePacket(
      const folly::IPAddressV6& dstIp,
      int priority = 0) {
    // Create a structured payload pattern.
    std::vector<uint8_t> payload(512, 0);
    for (int i = 0; i < payload.size() / 2; ++i) {
      payload[i * 2] = i & 0xff;
      payload[i * 2 + 1] = (i >> 8) & 0xff;
    }

    return utility::makeUDPTxPacket(
        getSw(),
        utility::firstVlanID(getProgrammedState()),
        utility::kLocalCpuMac(),
        utility::getFirstInterfaceMac(getProgrammedState()),
        folly::IPAddressV6{"2401:2222:2222:2222:2222:2222:2222:2222"},
        dstIp,
        0x4444,
        0x5555,
        (priority * 8) << 2, // dscp is the 6 MSBs in TC
        255,
        std::move(payload));
  }

  std::unique_ptr<TxPacket> sendPackets(
      int count,
      std::optional<PortID> portId,
      const folly::IPAddressV6& dstIp,
      int priority = 0) {
    auto pkt = makePacket(dstIp, priority);
    XLOG(DBG3) << "Sending " << count << " packets:\n"
               << PktUtil::hexDump(pkt->buf());
    for (int i = 0; i < count; ++i) {
      if (portId.has_value()) {
        getAgentEnsemble()->sendPacketAsync(
            makePacket(dstIp, priority), PortDescriptor(*portId), std::nullopt);
      } else {
        getAgentEnsemble()->sendPacketAsync(
            makePacket(dstIp, priority), std::nullopt, std::nullopt);
      }
    }
    return pkt; // return for later comparison
  }

  int32_t makeSSPA(PortID portId) {
    // Source System Port Aggregate is simply the system port ID.
    return static_cast<int32_t>(getSystemPortID(
        portId,
        getProgrammedState(),
        scopeResolver().scope(portId).switchId()));
  }

  int32_t makePortDSPA(PortID portId) {
    // Destination System Port Aggregate for ports is prefixed with 0x0C.
    return (0x0C << 16) | makeSSPA(portId);
  }

  int32_t makeTrapDSPA(int trapId) {
    return (0x10 << 16) | trapId;
  }

  void validateMirrorOnDropPacket(
      folly::IOBuf* recvBuf,
      folly::IOBuf* sentBuf,
      cfg::PortType portType,
      int8_t eventId,
      int32_t sspa, // source system port aggregate
      int32_t dspa, // destination system port aggregate
      int payloadOffset) {
    // Validate Ethernet header
    folly::io::Cursor recvCursor{recvBuf};
    utility::EthFrame frame{recvCursor};
    EXPECT_EQ(frame.header().getDstMac(), kCollectorNextHopMac_);

    // Validate IPv6 header
    EXPECT_TRUE(frame.v6PayLoad().has_value());
    if (!frame.v6PayLoad().has_value()) {
      return; // avoid causing a segfault which makes the log hard to read
    }
    auto ipHeader = frame.v6PayLoad()->header();
    EXPECT_EQ(ipHeader.dstAddr, kCollectorIp_);

    // Validate UDP header
    EXPECT_TRUE(frame.v6PayLoad()->udpPayload().has_value());
    if (!frame.v6PayLoad()->udpPayload().has_value()) {
      return; // avoid causing a segfault which makes the log hard to read
    }
    auto udpHeader = frame.v6PayLoad()->udpPayload()->header();
    EXPECT_EQ(udpHeader.srcPort, kMirrorSrcPort);
    EXPECT_EQ(udpHeader.dstPort, kMirrorDstPort);

    auto content = frame.v6PayLoad()->udpPayload()->payload();
    auto contentBuf =
        folly::IOBuf::wrapBufferAsValue(content.data(), content.size());
    folly::io::Cursor cursor(&contentBuf);

    if (portType == cfg::PortType::EVENTOR_PORT) {
      // When mirroring through eventor port, an extra 12 byte eventor header
      // will be added after MOD header.
      cursor.skip(12); // eventor sequence number, timestamp, sample
    }

    // Validate MoD header
    cursor.skip(3); // version, reserved fields
    EXPECT_EQ(cursor.readBE<int8_t>(), eventId);
    cursor.skip(4); // sequence number
    cursor.skip(8); // timestamp
    EXPECT_EQ(cursor.readBE<int32_t>(), sspa);
    EXPECT_EQ(cursor.readBE<int32_t>(), dspa);
    cursor.skip(8); // reserved fields

    // Eventor port limits the payload to 128 bytes, including the MOD header,
    // so original packet payload is truncated to 96 bytes.
    int truncateSize = portType == cfg::PortType::EVENTOR_PORT
        ? kEventorPayloadSize
        : kTruncateSize;
    EXPECT_EQ(cursor.totalLength(), truncateSize);
    auto payload = folly::IOBuf::copyBuffer(cursor.data(), truncateSize);
    // Due to truncation, the original packet is expected to be longer than
    // the mirror, so trim the original to match.
    sentBuf->trimEnd(sentBuf->length() - truncateSize);
    // In some cases the mirrored payload could differ from the sent packet,
    // e.g. MAC can change when traffic is looped back externally. Chop some
    // bytes from the start if needed.
    payload->trimStart(payloadOffset);
    sentBuf->trimStart(payloadOffset);
    // Do a string comparison for better readability.
    std::string payloadHex = folly::hexlify(payload->coalesce());
    std::string sentHex = folly::hexlify(sentBuf->coalesce());
    EXPECT_EQ(payloadHex, sentHex);
  }
};

INSTANTIATE_TEST_SUITE_P(
    AgentMirrorOnDropTest,
    AgentMirrorOnDropTest,
    testing::Values(cfg::PortType::RECYCLE_PORT, cfg::PortType::EVENTOR_PORT),
    [](const ::testing::TestParamInfo<cfg::PortType>& info) {
      return apache::thrift::util::enumNameSafe(info.param);
    });

TEST_P(AgentMirrorOnDropTest, ConfigChangePostWarmboot) {
  auto mirrorPortId1 = masterLogicalPortIds({cfg::PortType::RECYCLE_PORT})[1];
  auto mirrorPortId2 = masterLogicalPortIds({GetParam()})[0];
  XLOG(DBG3) << "MoD port 1: " << portDesc(mirrorPortId1);
  XLOG(DBG3) << "MoD port 2: " << portDesc(mirrorPortId2);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId1,
        kCollectorIp_,
        {{0,
          makeEventConfig(
              std::nullopt, // use default aging group
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_PACKET_PROCESSING_DISCARDS})}});
    applyNewConfig(config);
  };

  auto setupWb = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    config.mirrorOnDropReports()->clear();
    setupMirrorOnDrop(
        &config,
        mirrorPortId2,
        kCollectorIp_,
        {{0,
          makeEventConfig(
              cfg::MirrorOnDropAgingGroup::PORT,
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_PACKET_PROCESSING_DISCARDS})}});
    applyNewConfig(config);
  };

  verifyAcrossWarmBoots(setup, []() {}, setupWb, []() {});
}

TEST_P(AgentMirrorOnDropTest, ModWithSflowMirrorPresent) {
  auto mirrorPortId = masterLogicalPortIds({GetParam()})[0];
  auto sampledPortId = masterLogicalInterfacePortIds()[1];
  auto sflowPortId = masterLogicalInterfacePortIds()[2];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Sampled port: " << portDesc(sampledPortId);
  XLOG(DBG3) << "sFlow destination port: " << portDesc(sflowPortId);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    configureMirror(config);
    utility::configureSflowSampling(config, kSflowMirror, {sampledPortId}, 1);
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{0,
          makeEventConfig(
              std::nullopt,
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_PACKET_PROCESSING_DISCARDS})}});
    applyNewConfig(config);
    resolveRouteForMirrorDestination(
        sflowPortId, cfg::PortType::INTERFACE_PORT);

    config.mirrorOnDropReports()->clear();
    applyNewConfig(config);
  };

  verifyAcrossWarmBoots(setup, []() {});
}

TEST_P(AgentMirrorOnDropTest, PacketProcessingError) {
  auto mirrorPortId = masterLogicalPortIds({GetParam()})[0];
  auto injectionPortId = masterLogicalInterfacePortIds()[0];
  auto collectorPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  const int kEventId = 1;
  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{kEventId,
          makeEventConfig(
              std::nullopt, // use default aging group
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_PACKET_PROCESSING_DISCARDS})}});
    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    auto pkt = sendPackets(
        1,
        FLAGS_mod_pp_test_use_pipeline_lookup
            ? std::nullopt
            : std::make_optional<>(injectionPortId),
        kDropDestIp);

    WITH_RETRIES_N(3, {
      XLOG(DBG3) << "Waiting for mirror packet...";
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());

      if (frameRx.has_value()) {
        XLOG(DBG3) << PktUtil::hexDump(frameRx->get());
        validateMirrorOnDropPacket(
            frameRx->get(),
            pkt->buf(),
            GetParam(),
            kEventId,
            makeSSPA(injectionPortId),
            makeTrapDSPA(kRouteMissingTrapID),
            0 /*payloadOffset*/);
      }
    });

    // See if ACL counters increased.
    auto [maxRetryCount, sleepTimeMsecs] =
        utility::getRetryCountAndDelay(getSw()->getHwAsicTable());
    WITH_RETRIES_N_TIMED(
        maxRetryCount, std::chrono::milliseconds(sleepTimeMsecs), {
          EXPECT_EVENTUALLY_GT(
              utility::getAclInOutPackets(getSw(), kModPacketAclCouter), 0);
        });
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_P(AgentMirrorOnDropTest, MultipleEventIDs) {
  auto mirrorPortId = masterLogicalPortIds({GetParam()})[0];
  auto injectionPortId = masterLogicalInterfacePortIds()[0];
  auto collectorPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  const int kEventId = 1; // event ID for PP drops
  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        // Sandwich PP drops between other reasons and see if it still works.
        // Ensure all aging group tests are tested.
        // Also make sure to test multiple reason aggregations in one event.
        {
            {0,
             makeEventConfig(
                 cfg::MirrorOnDropAgingGroup::GLOBAL,
                 {cfg::MirrorOnDropReasonAggregation::
                      INGRESS_SOURCE_CONGESTION_DISCARDS})},
            {1,
             makeEventConfig(
                 cfg::MirrorOnDropAgingGroup::PORT,
                 {cfg::MirrorOnDropReasonAggregation::
                      INGRESS_PACKET_PROCESSING_DISCARDS,
                  cfg::MirrorOnDropReasonAggregation::
                      INGRESS_QUEUE_RESOLUTION_DISCARDS})},
            {2,
             makeEventConfig(
                 cfg::MirrorOnDropAgingGroup::VOQ,
                 {cfg::MirrorOnDropReasonAggregation::
                      INGRESS_DESTINATION_CONGESTION_DISCARDS})},
            {3,
             makeEventConfig(
                 cfg::MirrorOnDropAgingGroup::PRIORITY_GROUP,
                 {
                     cfg::MirrorOnDropReasonAggregation::INGRESS_MISC_DISCARDS,
                     cfg::MirrorOnDropReasonAggregation::
                         UNEXPECTED_REASON_DISCARDS,
                 })},
        });
    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    auto pkt = sendPackets(1, injectionPortId, kDropDestIp);

    WITH_RETRIES_N(3, {
      XLOG(DBG3) << "Waiting for mirror packet...";
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());

      if (frameRx.has_value()) {
        XLOG(DBG3) << PktUtil::hexDump(frameRx->get());
        validateMirrorOnDropPacket(
            frameRx->get(),
            pkt->buf(),
            GetParam(),
            kEventId,
            makeSSPA(injectionPortId),
            makeTrapDSPA(kRouteMissingTrapID),
            0 /*payloadOffset*/);
      }
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
