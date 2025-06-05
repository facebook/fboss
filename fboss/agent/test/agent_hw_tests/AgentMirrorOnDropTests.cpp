// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/MacAddress.h>
#include <gtest/gtest.h>
#include "fboss/agent/AsicUtils.h"
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
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/CommonUtils.h"

// TODO(maxgg): remove once CS00012385223 is resolved
DEFINE_bool(
    mod_pp_test_use_pipeline_lookup,
    false,
    "Use pipeline lookup mode in packet processing drop test");

const std::string kSflowMirror = "sflow_mirror";
const std::string kModPacketAcl = "mod_packet_acl";
const std::string kModPacketAclCounter = "mod_packet_acl_counter";

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
    // Add a remote DSF node for use in VoqReject test.
    config.dsfNodes() = *utility::addRemoteIntfNodeCfg(*config.dsfNodes(), 1);
    return config;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::MIRROR_ON_DROP};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // For VsqWordsSharedMaxSize which relies on triggering PFC.
    FLAGS_allow_zero_headroom_for_lossless_pg = true;
    // Allow using NIF port for MOD, to test failure recovery cases.
    FLAGS_allow_nif_port_for_mod = true;
  }

  void TearDown() override {
    // Log drop counters to facilitate test debugging.
    if (auto ensemble = getAgentEnsemble(); ensemble != nullptr) {
      std::string output;
      for (const auto& switchIdx : getSw()->getHwAsicTable()->getSwitchIDs()) {
        ensemble->runDiagCommand("quit\n", output, switchIdx);
      }
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
  const int kL3DropTrapId = 0x1B;
  const int kAclDropTrapId = 0x1C;
  const int kNoDestFoundTrapId = 0xB5;

  // Test constants. We switch the event IDs around to get test coverage.
  const int kL3DropEventId = 0;
  const int kNullRouteDropEventId = 1;
  const int kAclDropEventId = 2;
  const int kVsqDropEventId = 3;
  const int kVoqDropEventId = 4;

  PortID findRecirculationPort(cfg::PortType portType, int offset = 0) {
    std::vector<PortID> eligiblePortIds;
    const auto& cfg = getAgentEnsemble()->getCurrentConfig();
    for (const auto& port : *cfg.ports()) {
      if (port.portType() == portType && port.scope() == cfg::Scope::LOCAL) {
        eligiblePortIds.emplace_back(*port.logicalID());
      }
    }
    if (offset > eligiblePortIds.size()) {
      throw FbossError(
          "Offset ",
          offset,
          " exceeds the number of eligible ports in config: ",
          eligiblePortIds.size());
    }
    return eligiblePortIds[offset];
  }

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
        getProgrammedState(), getSw()->needL2EntryForNeighbor(), nextHopMac};
    const PortDescriptor port{portId};
    RoutePrefixV6 route{addr, 128};
    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, {port});
    });
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&routeUpdater, {port}, {std::move(route)});
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
    auto* acl = utility::addAcl_DEPRECATED(config, kModPacketAcl);
    acl->dstIp() = kCollectorIp_.str();
    acl->l4DstPort() = kMirrorDstPort;
    utility::addAclStat(
        config,
        kModPacketAcl,
        kModPacketAclCounter,
        utility::getAclCounterTypes(getAgentEnsemble()->getL3Asics()));
  }

  void addDropPacketAcl(cfg::SwitchConfig* config, const PortID& portId) {
    auto* acl = utility::addAcl_DEPRECATED(
        config,
        fmt::format("drop-packet-{}", portId),
        cfg::AclActionType::DENY);
    acl->etherType() = cfg::EtherType::IPv6;
    acl->srcPort() = portId;
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
              state,
              getSw()->needL2EntryForNeighbor(),
              RouterID(0),
              {portType});
          auto newState = ecmpHelper.resolveNextHops(state, nhopPorts);
          return newState;
        },
        "resolve mirror nexthop");

    auto mirror = getSw()->getState()->getMirrors()->getNodeIf(kSflowMirror);
    auto dip = mirror->getDestinationIp();

    RoutePrefix<T> prefix(T(dip->str()), dip->bitCount());
    utility::EcmpSetupTargetedPorts<T> ecmpHelper(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        RouterID(0),
        {portType});

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
        getVlanIDForTx(),
        utility::kLocalCpuMac(),
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
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

  SystemPortID systemPortId(const PortID& portId) {
    return getSystemPortID(
        portId, getProgrammedState(), scopeResolver().scope(portId).switchId());
  }

  int32_t makeSSPA(const PortID& portId) {
    // Source System Port Aggregate is simply the system port ID.
    return static_cast<int32_t>(systemPortId(portId));
  }

  int32_t makePortDSPA(SystemPortID portId) {
    // Destination System Port Aggregate for ports is prefixed with 0x0C.
    return (0x0C << 16) | portId;
  }

  int32_t makePortDSPA(const PortID& portId) {
    return makePortDSPA(systemPortId(portId));
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
    EXPECT_EQ(cursor.readBE<int8_t>() >> 4, 0); // higher 4 bits = version
    cursor.skip(2); // reserved fields
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

  void validateModPacketReceived(
      utility::SwSwitchPacketSnooper& snooper,
      folly::IOBuf* sentBuf,
      cfg::PortType portType,
      int8_t eventId,
      int32_t sspa,
      int32_t dspa,
      int payloadOffset = 0) {
    WITH_RETRIES_N(3, {
      XLOG(DBG3) << "Waiting for mirror packet...";
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());

      if (frameRx.has_value()) {
        XLOG(DBG3) << PktUtil::hexDump(frameRx->get());
        validateMirrorOnDropPacket(
            frameRx->get(),
            sentBuf,
            portType,
            eventId,
            sspa,
            dspa,
            payloadOffset);
      }
    });
  }
};

class AgentMirrorOnDropMtuTest : public AgentMirrorOnDropTest {
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentMirrorOnDropTest::initialConfig(ensemble);
    config = addCoppConfig(ensemble, config);
    for (auto& port : *config.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT) {
        // Prod MTU is 9412, but there seems to be some limit in the maximum
        // allowed payload length, so use 9000 instead.
        port.maxFrameSize() = 9000;
      }
    }
    return config;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::MIRROR_ON_DROP,
        ProductionFeature::PORT_MTU_ERROR_TRAP,
    };
  }
};

// Verifies that enabling MOD on a recycle port doesn't interfere with
// normal operations of the MTU trap.
TEST_F(AgentMirrorOnDropMtuTest, MtuTrapStillWorks) {
  auto mirrorPortId = findRecirculationPort(cfg::PortType::RECYCLE_PORT);
  auto egressPortId = masterLogicalInterfacePortIds()[0];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Egress port: " << portDesc(egressPortId);

  const folly::IPAddressV6 kDestIp{"2401:4444:4444:4444:4444:4444:4444:4444"};
  const folly::MacAddress kNeighborMac{"02:44:44:44:44:44"};

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    // Enable MOD on a local RCY port, which belongs to one of the cores.
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{0,
          makeEventConfig(
              std::nullopt, // use default aging group
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_PACKET_PROCESSING_DISCARDS})}});
    applyNewConfig(config);
    setupEcmpTraffic(egressPortId, kDestIp, kNeighborMac);
  };

  auto verify = [&]() {
    // Send > MTU packets from all NIF ports to a destination. This will
    // trigger MTU traps on all the cores.
    for (const auto& portId : masterLogicalInterfacePortIds()) {
      auto pkt = utility::makeUDPTxPacket(
          getSw(),
          getVlanIDForTx(),
          utility::kLocalCpuMac(),
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
          folly::IPAddressV6{"2401:2222:2222:2222:2222:2222:2222:2222"},
          kDestIp,
          0x4444,
          0x5555,
          0,
          10,
          std::vector<uint8_t>(9100, 0xff)); // >= MTU
      getAgentEnsemble()->sendPacketAsync(
          std::move(pkt), PortDescriptor(portId), std::nullopt);
    }

    // We expect all MTU traps to still work.
    WITH_RETRIES_N(5, {
      int pkts = 0;
      for (const auto switchId : getSw()->getHwAsicTable()->getSwitchIDs()) {
        // MTU trap goes to kCoppLowPriQueueId
        pkts += utility::getCpuQueueInPackets(
            getSw(), switchId, utility::kCoppLowPriQueueId);
      }
      EXPECT_EVENTUALLY_GE(pkts, masterLogicalInterfacePortIds().size());
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

class AgentMirrorOnDropDestMacTest : public AgentMirrorOnDropTest {
  const std::string kModDestMacOverride = "02:33:33:33:33:33";

  void setCmdLineFlagOverrides() const override {
    AgentMirrorOnDropTest::setCmdLineFlagOverrides();

    // In the failed case, a packet egressing a NIF port formerly used for MOD
    // will have its dest MAC overwritten by the MOD MAC. The deafult MOD MAC
    // is the switch MAC (firstInterfaceMac), so when the overwritten packet
    // loops back, it will be routed in some weird way. To make the test easier
    // to understand, we'll override the MOD dest MAC with a special MAC.
    FLAGS_mod_dest_mac_override = kModDestMacOverride;
  }
};

// 1. Override MOD dest MAC to 02:33:33:33:33:33.
// 2. Configure MOD on NIF port 1 (not expected to happen in prod, we use a
//    flag to allow this in testing), then unconfigure it.
// 3. Point NIF port 1 to a neighbor with dest MAC 02:44:44:44:44:44.
// 4. Send a packet towards that neighbor from the pipeline.
// 5. We expect the packet to be sent out of NIF port 1 with the correct
//    neighbor dest MAC (instead of, say, using the old MOD dest MAC).
TEST_F(AgentMirrorOnDropDestMacTest, ModOnInterfacePortCleanup) {
  auto mirrorPortId = masterLogicalInterfacePortIds()[0];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);

  const folly::IPAddressV6 kDestIp{"2401:4444:4444:4444:4444:4444:4444:4444"};
  const folly::MacAddress kNeighborMac{"02:44:44:44:44:44"};

  // Setup MOD on a NIF port.
  auto config = getAgentEnsemble()->getCurrentConfig();
  setupMirrorOnDrop(
      &config,
      mirrorPortId,
      kCollectorIp_,
      {{kL3DropEventId,
        makeEventConfig(
            std::nullopt, // use default aging group
            {cfg::MirrorOnDropReasonAggregation::
                 INGRESS_PACKET_PROCESSING_DISCARDS})}});
  applyNewConfig(config);

  // Remove MOD, now point that NIF port to a neighbor.
  // Add a trap to allow packet snooping.
  config.mirrorOnDropReports()->clear();
  const HwAsic* asic = checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
  utility::addTrapPacketAcl(asic, &config, mirrorPortId);
  applyNewConfig(config);
  setupEcmpTraffic(mirrorPortId, kDestIp, kNeighborMac);

  // Send packet towards neighbor IP from the pipeline.
  utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
  auto pkt = sendPackets(1, std::nullopt, kDestIp);

  // Dump counters and tables first, since assertions may abort the test.
  std::string out;
  getAgentEnsemble()->runDiagCommand("\n", out);
  getAgentEnsemble()->runDiagCommand("show counters full\n", out);
  XLOG(INFO) << ">>> After sendPacket 2\n" << out;
  getAgentEnsemble()->runDiagCommand("dbal table dump table=ESEM_ARP\n", out);
  XLOG(INFO) << out;
  getAgentEnsemble()->runDiagCommand(
      "dbal table dump table=MAC_SOURCE_ADDRESS_FULL\n", out);
  XLOG(INFO) << out;

  WITH_RETRIES_N(3, {
    XLOG(DBG3) << "Waiting for NIF packet...";
    auto frameRx = snooper.waitForPacket(1);
    ASSERT_EVENTUALLY_TRUE(frameRx.has_value());
    XLOG(DBG3) << PktUtil::hexDump(frameRx->get());

    folly::io::Cursor recvCursor{frameRx->get()};
    utility::EthFrame frame{recvCursor};
    EXPECT_EQ(frame.header().getDstMac(), kNeighborMac);
  });
}

INSTANTIATE_TEST_SUITE_P(
    AgentMirrorOnDropTest,
    AgentMirrorOnDropTest,
    testing::Values(cfg::PortType::RECYCLE_PORT, cfg::PortType::EVENTOR_PORT),
    [](const ::testing::TestParamInfo<cfg::PortType>& info) {
      return apache::thrift::util::enumNameSafe(info.param);
    });

// 1. Configures MOD on a recycle or eventor port, but don't add a route to the
//    collector IP.
// 2. Sends a packet towards an unknown destination.
// 3. Check that recycle/eventor port stats increase by at most 2 packets.
TEST_P(AgentMirrorOnDropTest, NoInfiniteLoopRecirculated) {
  auto mirrorPortId = masterLogicalPortIds({GetParam()})[0];
  auto injectionPortId = masterLogicalInterfacePortIds()[0];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{kL3DropEventId,
          makeEventConfig(
              std::nullopt, // use default aging group
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_PACKET_PROCESSING_DISCARDS})}});
    applyNewConfig(config);
  };

  auto verify = [&]() {
    auto injectionPortStatsBefore = getLatestPortStats(injectionPortId);
    auto mirrorPortStatsBefore = getLatestPortStats(mirrorPortId);
    int queueOutBytesBefore = 0;
    for (const auto& [id, bytes] : *mirrorPortStatsBefore.queueOutBytes_()) {
      queueOutBytesBefore += bytes;
    }

    sendPackets(1, injectionPortId, kDropDestIp);

    // Ensure packet drop happened.
    WITH_RETRIES_N(5, {
      auto injectionPortStatsAfter = getLatestPortStats(injectionPortId);
      EXPECT_EVENTUALLY_GT(
          *injectionPortStatsAfter.inDstNullDiscards_(),
          *injectionPortStatsBefore.inDstNullDiscards_());
    });

    auto mirrorPortStatsAfter = getLatestPortStats(mirrorPortId);

    // At most 2 packets (one MOD packet for the original, one MOD packet for
    // the dropped MOD packet).
    EXPECT_LE(
        *mirrorPortStatsAfter.outUnicastPkts_() -
            *mirrorPortStatsBefore.outUnicastPkts_(),
        2);

    // MOD payload is truncated to 128 bytes, with MOD/UDP/IP/Eth header each
    // packet should be just a little over 200 bytes.
    int queueOutBytesAfter = 0;
    for (const auto& [id, bytes] : *mirrorPortStatsAfter.queueOutBytes_()) {
      queueOutBytesAfter += bytes;
    }
    EXPECT_LE(queueOutBytesAfter - queueOutBytesBefore, 500);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Verifies that changing MOD configs after warmboot succeeds.
TEST_P(AgentMirrorOnDropTest, ConfigChangePostWarmboot) {
  auto mirrorPortId1 = findRecirculationPort(cfg::PortType::RECYCLE_PORT, 1);
  auto mirrorPortId2 = findRecirculationPort(GetParam(), 0);
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

// Verifies that changing MOD config when sFlow is enabled succeeds
// (CS00012385636).
TEST_P(AgentMirrorOnDropTest, ModWithSflowMirrorPresent) {
  auto mirrorPortId = findRecirculationPort(GetParam());
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

// 1. Configures MOD with packet processing drops mapped to event ID
//    `kL3DropEventId`.
// 2. Sends a packet towards an unknown destination.
// 3. Packet will be dropped and a MOD packet will be sent to collector.
// 4. Packets are trapped to CPU. We validate the MOD headers well as a
//    truncated version of the original packet.
TEST_P(AgentMirrorOnDropTest, PacketProcessingDefaultRouteDrop) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto injectionPortId = masterLogicalInterfacePortIds()[0];
  auto collectorPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{kL3DropEventId,
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
    validateModPacketReceived(
        snooper,
        pkt->buf(),
        GetParam(),
        kL3DropEventId,
        makeSSPA(injectionPortId),
        makeTrapDSPA(kL3DropTrapId));

    // See if ACL counters increased.
    auto [maxRetryCount, sleepTimeMsecs] =
        utility::getRetryCountAndDelay(getSw()->getHwAsicTable());
    WITH_RETRIES_N_TIMED(
        maxRetryCount, std::chrono::milliseconds(sleepTimeMsecs), {
          EXPECT_EVENTUALLY_GT(
              utility::getAclInOutPackets(getSw(), kModPacketAclCounter), 0);
        });
  };

  verifyAcrossWarmBoots(setup, verify);
}

// 1. Configure MoD with packet processing discards/drops mapped to event ID
//    `kNullRouteDropEventId`.
// 2. Delete the null route so the packet does not match any route and is
//    dropped.
// 3. Sends a packet towards a destination not matching any route.
// 3. Packet will be dropped and a MOD packet will be sent to collector.
// 4. Packets are trapped to CPU. We validate the MOD headers well as a
//    truncated version of the original packet.
TEST_P(AgentMirrorOnDropTest, PacketProcessingNullRouteDrop) {
  PortID mirrorPortId = findRecirculationPort(GetParam());
  PortID injectionPortId = masterLogicalInterfacePortIds()[0];
  PortID collectorPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{kNullRouteDropEventId,
          makeEventConfig(
              std::nullopt, // use default aging group
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_PACKET_PROCESSING_DISCARDS})}});
    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
  };

  auto verify = [&]() {
    // Delete null route so that we hit the trap drop instead of null route
    // drop. We do this in verify since we unconditionally install null routes
    // on both Warm/Cold boots. And removing null routes in setup, would just
    // cause them to be restored in WB path. This then would fail the test and
    // also trigger unexpected WB writes.
    SwSwitchRouteUpdateWrapper routeUpdater = getSw()->getRouteUpdater();
    routeUpdater.delRoute(
        RouterID(0), folly::IPAddress("::"), 0, ClientID::STATIC_INTERNAL);
    routeUpdater.delRoute(
        RouterID(0), folly::IPAddress("0.0.0.0"), 0, ClientID::STATIC_INTERNAL);
    routeUpdater.program();

    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    std::unique_ptr<TxPacket> pkt = sendPackets(
        1,
        FLAGS_mod_pp_test_use_pipeline_lookup
            ? std::nullopt
            : std::make_optional<>(injectionPortId),
        kDropDestIp);
    validateModPacketReceived(
        snooper,
        pkt->buf(),
        GetParam(),
        kNullRouteDropEventId,
        makeSSPA(injectionPortId),
        makeTrapDSPA(kNoDestFoundTrapId));

    auto [maxRetryCount, sleepTimeMsecs] =
        utility::getRetryCountAndDelay(getSw()->getHwAsicTable());
    WITH_RETRIES_N_TIMED(
        maxRetryCount, std::chrono::milliseconds(sleepTimeMsecs), {
          EXPECT_EVENTUALLY_GT(
              utility::getAclInOutPackets(getSw(), kModPacketAclCounter), 0);
        });
  };

  verifyAcrossWarmBoots(setup, verify);
}

// 1. Configures MOD with packet processing drops mapped to event ID
//    `kAclDropEventId`. Add an ACL entry on a port to drop all packets.
// 2. Sends a packet out of that port and let it loop back.
// 3. Packet will be dropped and a MOD packet will be sent to collector.
// 4. Packets are trapped to CPU. We validate the MOD headers well as a
//    truncated version of the original packet.
TEST_P(AgentMirrorOnDropTest, PacketProcessingAclDrop) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto injectionPortId = masterLogicalInterfacePortIds()[0];
  auto collectorPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{kAclDropEventId,
          makeEventConfig(
              std::nullopt, // use default aging group
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_PACKET_PROCESSING_DISCARDS})}});
    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    addDropPacketAcl(&config, injectionPortId);
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    auto pkt = sendPackets(1, injectionPortId, kDropDestIp);
    validateModPacketReceived(
        snooper,
        pkt->buf(),
        GetParam(),
        kAclDropEventId,
        makeSSPA(injectionPortId),
        makeTrapDSPA(kAclDropTrapId));
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Map the drop reason we are looking for (packet process drops) to an
// event ID sandwiched between other event IDs. Then perform the same test as
// PacketProcessingDefaultRouteDrop. We expect everything to still work.
TEST_P(AgentMirrorOnDropTest, MultipleEventIDs) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto injectionPortId = masterLogicalInterfacePortIds()[0];
  auto collectorPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {
            {0,
             makeEventConfig(
                 cfg::MirrorOnDropAgingGroup::GLOBAL,
                 {cfg::MirrorOnDropReasonAggregation::
                      INGRESS_SOURCE_CONGESTION_DISCARDS})},
            {1,
             // Also test multiple reason aggregations in one event
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
    validateModPacketReceived(
        snooper,
        pkt->buf(),
        GetParam(),
        1, // for INGRESS_PACKET_PROCESSING_DISCARDS
        makeSSPA(injectionPortId),
        makeTrapDSPA(kL3DropTrapId));
  };

  verifyAcrossWarmBoots(setup, verify);
}

// 1. Configures MOD with VOQ drops mapped to event ID `kVoqDropEventId`.
// 2. Setup a remote DSF node in initialConfig(). Add a remote system port and
//    add an neighbor on the corresponding interface.
// 3. Disable credit watchdog so that packets in VOQ don't expire.
// 4. Sends a bunch of packets towards the neighbor. Packets will be stuck in
//    VOQ and cause tail drops.
// 5. MOD packet will be sent to collector.
// 6. Packets are trapped to CPU. We validate the MOD headers well as a
//    truncated version of the original packet.
TEST_P(AgentMirrorOnDropTest, VoqReject) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto collectorPortId = masterLogicalInterfacePortIds()[0];
  auto ingressPortId = masterLogicalInterfacePortIds()[1];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);
  XLOG(DBG3) << "Ingress port: " << portDesc(ingressPortId);

  constexpr auto remotePortId = 401;
  const SystemPortID kRemoteSysPortId(remotePortId);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{kVoqDropEventId,
          makeEventConfig(
              cfg::MirrorOnDropAgingGroup::GLOBAL,
              {cfg::MirrorOnDropReasonAggregation::
                   INGRESS_DESTINATION_CONGESTION_DISCARDS})}});
    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);

    // Add neighbor on remote system port. The remoteSwitchId formula must be
    // in sync with the logic in addRemoteIntfNodeCfg().
    SwitchID remoteSwitchId = static_cast<SwitchID>(
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getNumCores() *
        getAgentEnsemble()->getNumL3Asics());
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in, scopeResolver(), kRemoteSysPortId, remoteSwitchId);
    });
    const InterfaceID kIntfId(remotePortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in, scopeResolver(), kIntfId, {{kDropDestIp, 128}});
    });
    const PortDescriptor kPort(kRemoteSysPortId);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoveRemoteNeighbor(
          in,
          scopeResolver(),
          kDropDestIp,
          kIntfId,
          kPort,
          true /*add*/,
          utility::getDummyEncapIndex(getAgentEnsemble()));
    });

    // Disable credit watchdog so that packets in VOQ don't expire.
    utility::enableCreditWatchdog(getAgentEnsemble(), false);
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");

    WITH_RETRIES({
      // We don't know what the VOQ limits are, so keep sending packets.
      auto pkt = sendPackets(1000, ingressPortId, kDropDestIp);

      XLOG(DBG3) << "Waiting for mirror packet...";
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());

      if (frameRx.has_value()) {
        XLOG(DBG3) << PktUtil::hexDump(frameRx->get());
        validateMirrorOnDropPacket(
            frameRx->get(),
            pkt->buf(),
            GetParam(),
            kVoqDropEventId,
            makeSSPA(ingressPortId),
            makePortDSPA(kRemoteSysPortId),
            0 /*payloadOffset*/);
      }
    });

    // This test generates many drops and we must consume them all, otherwise
    // PacketSnooper will complain on shutdown.
    while (snooper.waitForPacket(1).has_value()) {
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

// 1. Configures MOD with VSQ drops mapped to event ID `kVsqDropEventId`.
// 2. Configure PFC on an ingress port and set a small buffer size.
// 3. Add a route to a neighbor IP on an egress port. Disable Tx on that port.
// 4. Sends a bunch of packets towards the neighbor. Packets will be rejected
//    once ingress buffers fill up.
// 5. MOD packet will be sent to collector.
// 6. Packets are trapped to CPU. We validate the MOD headers well as a
//    truncated version of the original packet.
TEST_P(AgentMirrorOnDropTest, VsqReject) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto collectorPortId = masterLogicalInterfacePortIds()[0];
  auto injectionPortId = masterLogicalInterfacePortIds()[1];
  auto txOffPortId = masterLogicalInterfacePortIds()[2];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  XLOG(DBG3) << "Tx off port: " << portDesc(txOffPortId);
  int kPriority = 2;

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {
            {kVsqDropEventId,
             makeEventConfig(
                 cfg::MirrorOnDropAgingGroup::GLOBAL,
                 {cfg::MirrorOnDropReasonAggregation::
                      INGRESS_SOURCE_CONGESTION_DISCARDS})},
        });
    utility::setupPfcBuffers(
        getAgentEnsemble(),
        config,
        {injectionPortId},
        {kPriority},
        {} /*lossyPgIds*/,
        {} /*tcToPgOverride*/,
        utility::PfcBufferParams{
            .globalShared = 5000,
            .globalHeadroom = 0,
            .pgHeadroom = 0,
            .scalingFactor = cfg::MMUScalingFactor::ONE_128TH,
        });
    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    setupEcmpTraffic(
        txOffPortId,
        kDropDestIp,
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState()));
  };

  auto verify = [&]() {
    utility::setCreditWatchdogAndPortTx(getAgentEnsemble(), txOffPortId, false);
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    auto pkt = sendPackets(1000, injectionPortId, kDropDestIp, kPriority);

    validateModPacketReceived(
        snooper,
        pkt->buf(),
        GetParam(),
        kVsqDropEventId,
        makeSSPA(injectionPortId),
        makePortDSPA(txOffPortId));

    // This test generates a ton of drops, so consume the rest of MOD packets.
    while (snooper.waitForPacket(1).has_value()) {
    }

    utility::setCreditWatchdogAndPortTx(getAgentEnsemble(), txOffPortId, true);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
