// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/MacAddress.h>
#include <gtest/gtest.h>
#include <algorithm>
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/packet/bcm/XgsPsampMod.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/CommonUtils.h"

// TODO(maxgg): remove once CS00012385223 is resolved
DEFINE_bool(
    mod_pp_test_use_pipeline_lookup,
    false,
    "Use pipeline lookup mode in packet processing drop test");
DEFINE_int32(
    mod_num_mirrors,
    4,
    "Number of mirrors to create in ModWithMultipleMirrors");
DEFINE_int32(mod_num_sflow, 1, "Number of sFlow traffic sources");

const std::string kSflowMirror = "sflow_mirror";
const std::string kModPacketAcl = "mod_packet_acl";
const std::string kModPacketAclCounter = "mod_packet_acl_counter";

namespace facebook::fboss {

using namespace ::testing;

enum class DroppedPacketType { NONE, NIF, CPU };

// Base class for Mirror on Drop tests.
// Flow of the test:
// 1. Setup mirror on drop, using a Recycle port as the MoD port.
// 2. Pick an Interface port and add a route to the MoD collector IP.
// 3. Trigger some packet drops.
// 4. A MoD packet will be generated and injected into the port picked in (1).
// 5. The MoD packet will be routed and sent out of the port picked in (2).
// 6. The MoD packet will be looped back and punted to the CPU by ACL.
// 7. We capture the packet using PacketSnooper and verify the contents.
class AgentMirrorOnDropTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    // To allow infinite traffic loops.
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::MIRROR_ON_DROP};
  }

 protected:
  // Packet to be dropped
  const folly::IPAddressV6 kDropDestIp{
      "2401:3333:3333:3333:3333:3333:3333:3333"};

  // MOD/collector addresses
  const folly::MacAddress kCollectorNextHopMac_{"02:88:88:88:88:88"};
  const folly::IPAddressV6 kCollectorIp_{
      "2401:9999:9999:9999:9999:9999:9999:9999"};
  const folly::IPAddressV6 kSwitchIp_{"2401:face:b00c::01"};
  const int16_t kMirrorSrcPort = 0x6666;
  const int16_t kMirrorDstPort = 0x7777;

  // Packet fields used in makePacket() — used to verify inner packet in MoD
  const folly::IPAddressV6 kPacketSrcIp_{
      "2401:2222:2222:2222:2222:2222:2222:2222"};
  static constexpr uint16_t kPacketSrcPort = 0x4444;
  static constexpr uint16_t kPacketDstPort = 0x5555;

  std::string portDesc(const PortID& portId) {
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
      const PortID& portId,
      const folly::IPAddressV6& addr,
      const folly::MacAddress& nextHopMac,
      bool disableTtlDecrement = false) {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(), getSw()->needL2EntryForNeighbor(), nextHopMac};
    const PortDescriptor port{portId};
    RoutePrefixV6 route{addr, 128};
    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, {port});
    });
    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater,
        {port},
        {std::move(route)},
        {},
        std::nullopt,
        disableTtlDecrement ? std::make_optional(true) : std::nullopt);
    if (disableTtlDecrement) {
      for (auto& nhop : ecmpHelper.getNextHops()) {
        utility::disablePortTTLDecrementIfSupported(
            getAgentEnsemble(), ecmpHelper.getRouterId(), nhop);
      }
    }
  }

  void addDropPacketAcl(cfg::SwitchConfig* config, const PortID& portId) {
    auto* acl = utility::addAcl_DEPRECATED(
        config,
        fmt::format("drop-packet-{}", portId),
        cfg::AclActionType::DENY);
    acl->etherType() = cfg::EtherType::IPv6;
    acl->srcPort() = portId;
  }

  // Create a packet that will be dropped. The entire packet is expected to be
  // copied to the MoD payload. However note that if the test involves looping
  // traffic before they are dropped (e.g. by triggering buffer exhaustion), the
  // src/dst MAC defined here may be different from the MoD payload.
  std::unique_ptr<TxPacket> makePacket(
      const folly::IPAddressV6& dstIp,
      int priority = 0,
      size_t payloadSize = 512) {
    // Create a structured payload pattern.
    std::vector<uint8_t> payload(payloadSize, 0);
    for (int i = 0; i < payload.size() / 2; ++i) {
      payload[i * 2] = i & 0xff;
      payload[i * 2 + 1] = (i >> 8) & 0xff;
    }

    return utility::makeUDPTxPacket(
        getSw(),
        getVlanIDForTx(),
        utility::kLocalCpuMac(),
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
        kPacketSrcIp_,
        dstIp,
        kPacketSrcPort,
        kPacketDstPort,
        (priority * 8) << 2, // dscp is the 6 MSBs in TC
        255,
        std::move(payload));
  }

  std::unique_ptr<TxPacket> sendPackets(
      int count,
      std::optional<PortID> portId,
      const folly::IPAddressV6& dstIp,
      int priority = 0,
      size_t payloadSize = 512) {
    auto pkt = makePacket(dstIp, priority, payloadSize);
    XLOG(DBG3) << "Sending " << count << " packets:\n"
               << PktUtil::hexDump(pkt->buf());
    for (int i = 0; i < count; ++i) {
      if (portId.has_value()) {
        getAgentEnsemble()->sendPacketAsync(
            makePacket(dstIp, priority, payloadSize),
            PortDescriptor(*portId),
            std::nullopt);
      } else {
        getAgentEnsemble()->sendPacketAsync(
            makePacket(dstIp, priority, payloadSize),
            std::nullopt,
            std::nullopt);
      }
    }
    return pkt; // return for later comparison
  }

  SystemPortID systemPortId(const PortID& portId) {
    return getSystemPortID(
        portId, getProgrammedState(), scopeResolver().scope(portId).switchId());
  }
};

// DNX/Jericho3 specific Mirror on Drop test class.
class AgentMirrorOnDropDnxTest
    : public AgentMirrorOnDropTest,
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
    // To allow infinite traffic loops.
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), config);
    return config;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::MIRROR_ON_DROP,
        ProductionFeature::MIRROR_ON_DROP_DNX};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // For VsqWordsSharedMaxSize which relies on triggering PFC.
    FLAGS_allow_zero_headroom_for_lossless_pg = true;
    // Allow using NIF port for MOD, to test failure recovery cases.
    FLAGS_allow_nif_port_for_mod = true;
  }

 protected:
  // MOD settings
  const int kLargeTruncateSize = 400;
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
  const int kPrecedenceDropEventId = 5;

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

  std::map<int8_t, cfg::MirrorOnDropEventConfig> prodModConfig() {
    return {
        {
            0,
            makeEventConfig(
                cfg::MirrorOnDropAgingGroup::GLOBAL,
                {cfg::MirrorOnDropReasonAggregation::
                     UNEXPECTED_REASON_DISCARDS}),
        },
        {
            1,
            makeEventConfig(
                cfg::MirrorOnDropAgingGroup::GLOBAL,
                {cfg::MirrorOnDropReasonAggregation::INGRESS_MISC_DISCARDS}),
        },
        {
            2,
            makeEventConfig(
                cfg::MirrorOnDropAgingGroup::PORT,
                {cfg::MirrorOnDropReasonAggregation::
                     INGRESS_PACKET_PROCESSING_DISCARDS}),
        },
        {
            3,
            makeEventConfig(
                cfg::MirrorOnDropAgingGroup::PORT,
                {cfg::MirrorOnDropReasonAggregation::
                     INGRESS_QUEUE_RESOLUTION_DISCARDS}),
        },
        {
            4,
            makeEventConfig(
                cfg::MirrorOnDropAgingGroup::PORT,
                {cfg::MirrorOnDropReasonAggregation::
                     INGRESS_SOURCE_CONGESTION_DISCARDS}),
        },
        {
            5,
            makeEventConfig(
                cfg::MirrorOnDropAgingGroup::VOQ,
                {cfg::MirrorOnDropReasonAggregation::
                     INGRESS_DESTINATION_CONGESTION_DISCARDS}),
        },
        {
            6,
            makeEventConfig(
                cfg::MirrorOnDropAgingGroup::GLOBAL,
                {cfg::MirrorOnDropReasonAggregation::
                     INGRESS_GLOBAL_CONGESTION_DISCARDS}),
        },
    };
  }

  void setupMirrorOnDrop(
      cfg::SwitchConfig* config,
      PortID portId,
      const folly::IPAddressV6& collectorIp,
      const std::map<int8_t, cfg::MirrorOnDropEventConfig>& modEventToConfigMap,
      const std::map<cfg::MirrorOnDropAgingGroup, int32_t>& agingIntervals = {},
      int truncateSize = 128) {
    cfg::MirrorOnDropReport report;
    report.name() = "mod-1";
    report.mirrorPortId() = portId;
    report.localSrcPort() = kMirrorSrcPort;
    report.collectorIp() = collectorIp.str();
    report.collectorPort() = kMirrorDstPort;
    report.mtu() = 1500;
    report.truncateSize() = truncateSize;
    report.dscp() = 0;
    report.modEventToConfigMap() = modEventToConfigMap;
    report.agingGroupAgingIntervalUsecs() = agingIntervals;
    if (report.agingGroupAgingIntervalUsecs()->empty()) {
      report
          .agingGroupAgingIntervalUsecs()[cfg::MirrorOnDropAgingGroup::GLOBAL] =
          100;
      report.agingGroupAgingIntervalUsecs()[cfg::MirrorOnDropAgingGroup::PORT] =
          200;
    }
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

  void addMirror(
      cfg::SwitchConfig* config,
      const PortID& srcPortId,
      const PortID& destPortId) {
    cfg::Mirror mirror;
    mirror.name() = fmt::format("mirror-{}", srcPortId);
    mirror.destination().ensure().egressPort().ensure().set_logicalID(
        destPortId);
    mirror.truncate() = true;
    mirror.samplingRate() = 1; // mirror all packets
    config->mirrors()->push_back(mirror);

    auto portConfig = utility::findCfgPort(*config, srcPortId);
    portConfig->ingressMirror() = *mirror.name();
  }

  void configureSflowMirror(cfg::SwitchConfig& cfg) const {
    utility::configureSflowMirror(
        cfg,
        kSflowMirror,
        true /*truncate*/,
        utility::getSflowMirrorDestination(false /*v4*/).str());
  }

  template <typename T = folly::IPAddressV6>
  void resolveRouteForMirrorDestination(
      PortID mirrorDestinationPort,
      std::set<cfg::PortType> portTypes) {
    boost::container::flat_set<PortDescriptor> nhopPorts{
        PortDescriptor(mirrorDestinationPort)};

    applyNewState(
        [&](const std::shared_ptr<SwitchState>& state) {
          utility::EcmpSetupTargetedPorts<T> ecmpHelper(
              state, getSw()->needL2EntryForNeighbor(), RouterID(0), portTypes);
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
        portTypes);

    ecmpHelper.programRoutes(
        getAgentEnsemble()->getRouteUpdaterWrapper(), nhopPorts, {prefix});
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

  DroppedPacketType validateMirrorOnDropPacket(
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
      return DroppedPacketType::NONE; // can't use ASSERT_TRUE in non-void func
    }
    auto ipHeader = frame.v6PayLoad()->header();
    EXPECT_EQ(ipHeader.dstAddr, kCollectorIp_);

    // Validate UDP header
    EXPECT_TRUE(frame.v6PayLoad()->udpPayload().has_value());
    if (!frame.v6PayLoad()->udpPayload().has_value()) {
      return DroppedPacketType::NONE; // can't use ASSERT_TRUE in non-void func
    }
    auto udpHeader = frame.v6PayLoad()->udpPayload()->header();
    EXPECT_EQ(udpHeader.srcPort, kMirrorSrcPort);
    EXPECT_EQ(udpHeader.dstPort, kMirrorDstPort);

    auto content = frame.v6PayLoad()->udpPayload()->payload();
    auto contentBuf =
        folly::IOBuf::wrapBufferAsValue(content.data(), content.size());
    folly::io::Cursor cursor(&contentBuf);

    // Eventor port limits the payload to 128 bytes, including the MOD header,
    // so original packet payload is truncated to 96 bytes.
    // Also, on 12.x, a 12-byte eventor header is added before MOD header.
    int truncateSize = kTruncateSize;
    if (portType == cfg::PortType::EVENTOR_PORT) {
      truncateSize = kEventorPayloadSize;
      cursor.skip(12); // eventor sequence number, timestamp, sample
    }

    // Validate MoD header
    DroppedPacketType droppedPacketType;
    EXPECT_EQ(cursor.readBE<int8_t>() >> 4, 0); // higher 4 bits = version
    cursor.skip(2); // reserved fields
    EXPECT_EQ(cursor.readBE<int8_t>(), eventId);
    cursor.skip(4); // sequence number
    cursor.skip(8); // timestamp
    int32_t gotSSPA = cursor.readBE<int32_t>();
    int32_t gotDSPA = cursor.readBE<int32_t>();
    if (gotSSPA == 0) {
      // An extra 21-byte system header is added for CPU packets.
      cursor.skip(21);
      truncateSize -= 21;
      droppedPacketType = DroppedPacketType::CPU;
    } else {
      EXPECT_EQ(gotSSPA, sspa);
      droppedPacketType = DroppedPacketType::NIF;
    }
    EXPECT_EQ(gotDSPA, dspa);
    cursor.skip(8); // reserved fields

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

    return droppedPacketType;
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

  std::vector<PortID> portIdsToTest() {
    if (FLAGS_hyper_port) {
      return masterLogicalHyperPortIds();
    }
    return masterLogicalInterfacePortIds();
  }

  uint64_t getExpectedLineRate(const PortID& portId) {
    uint64_t speed =
        static_cast<uint64_t>(
            getProgrammedState()->getPorts()->getNodeIf(portId)->getSpeed()) *
        1000 * 1000;
    if (FLAGS_hyper_port) {
      // One hyper port per core, each core processing power is 1.35G pps.
      // Since mirror created duplicated packets, max processing power
      // is 1.35G / 2 = 0.675G pps. Besides, core process moves from processing
      // one full packet per clock to only partial packet segment per clock, if
      // packet size is larger than 513 bytes. In this test, UDP packet of
      // payload 512 bytes are used. Thus, each packet has 2 segements. Core
      // processing power further reduced to 0.675G pps / 2 = 0.3375G pps.
      // Traffic rate is: 0.3375G pps * (14 (eth header) + 2 (hyper port header)
      // + 40 (ipv6 header) + 8 (udp header) + 512 (payload) bytes) * 8 bit/byte
      // = ~1.6T bps

      // In another manual test with 400 UDP payload, each packet can fill into
      // one segment. Theoretical traffic rate is thus: 0.675G pps * (14 (eth
      // header) + 2 (hyper port header) + 40 (ipv6 header) + 8 (udp header) +
      // 400 (payload) bytes) * 8 bit/byte = ~2.6T bps This theoretical rate is
      // consistent with what we actually monitored.
      return 0.5 * speed;
    }
    return 0.9 * speed;
  }
};

class AgentMirrorOnDropMtuTest : public AgentMirrorOnDropDnxTest {
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentMirrorOnDropDnxTest::initialConfig(ensemble);
    config = addCoppConfig(ensemble, config);
    for (auto& port : *config.ports()) {
      if (port.portType() == cfg::PortType::INTERFACE_PORT ||
          port.portType() == cfg::PortType::HYPER_PORT) {
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
        ProductionFeature::MIRROR_ON_DROP_DNX,
        ProductionFeature::PORT_MTU_ERROR_TRAP,
    };
  }
};

// Verifies that enabling MOD on a recycle port doesn't interfere with
// normal operations of the MTU trap.
TEST_F(AgentMirrorOnDropMtuTest, MtuTrapStillWorks) {
  auto mirrorPortId = findRecirculationPort(cfg::PortType::RECYCLE_PORT);
  auto egressPortId = portIdsToTest()[0];
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
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    // Send > MTU packets from all NIF ports to a destination. This will
    // trigger MTU traps on all the cores.
    for (const auto& portId : portIdsToTest()) {
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
      EXPECT_EVENTUALLY_GE(pkts, portIdsToTest().size());
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

class AgentMirrorOnDropDestMacTest : public AgentMirrorOnDropDnxTest {
  const std::string kModDestMacOverride = "02:33:33:33:33:33";

  void setCmdLineFlagOverrides() const override {
    AgentMirrorOnDropDnxTest::setCmdLineFlagOverrides();

    // In the failed case, a packet egressing a NIF port formerly used for MOD
    // will have its dest MAC overwritten by the MOD MAC. The default MOD MAC
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
  auto mirrorPortId = portIdsToTest()[0];
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
  waitForStateUpdates(getSw());

  // Send packet towards neighbor IP from the pipeline.
  utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
  auto pkt = sendPackets(1, std::nullopt, kDestIp);

  // Dump counters and tables first, since assertions may abort the test.
  for (const auto& [switchId, _] : getAsics()) {
    std::string out;
    getAgentEnsemble()->runDiagCommand("\n", out, switchId);
    getAgentEnsemble()->runDiagCommand("show counters full\n", out, switchId);
    XLOG(INFO) << ">>> After sendPacket 2\n" << out;
    getAgentEnsemble()->runDiagCommand(
        "dbal table dump table=ESEM_ARP\n", out, switchId);
    XLOG(INFO) << out;
    getAgentEnsemble()->runDiagCommand(
        "dbal table dump table=MAC_SOURCE_ADDRESS_FULL\n", out, switchId);
    XLOG(INFO) << out;
  }

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
    AgentMirrorOnDropDnxTest,
    AgentMirrorOnDropDnxTest,
    testing::Values(cfg::PortType::RECYCLE_PORT, cfg::PortType::EVENTOR_PORT),
    [](const ::testing::TestParamInfo<cfg::PortType>& info) {
      return apache::thrift::util::enumNameSafe(info.param);
    });

// 1. Configures MOD on a recycle or eventor port, but don't add a route to the
//    collector IP.
// 2. Sends a packet towards an unknown destination.
// 3. Check that recycle/eventor port stats increase by at most 2 packets.
TEST_P(AgentMirrorOnDropDnxTest, NoInfiniteLoopRecirculated) {
  auto mirrorPortId = masterLogicalPortIds({GetParam()})[0];
  auto injectionPortId = portIdsToTest()[0];
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
    waitForStateUpdates(getSw());
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
TEST_P(AgentMirrorOnDropDnxTest, ConfigChangePostWarmboot) {
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
    waitForStateUpdates(getSw());
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
TEST_P(AgentMirrorOnDropDnxTest, ModWithSflowMirrorPresent) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto sampledPortId = portIdsToTest()[1];
  auto sflowPortId = portIdsToTest()[2];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Sampled port: " << portDesc(sampledPortId);
  XLOG(DBG3) << "sFlow destination port: " << portDesc(sflowPortId);

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    configureSflowMirror(config);
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
        sflowPortId,
        {cfg::PortType::INTERFACE_PORT, cfg::PortType::HYPER_PORT});

    config.mirrorOnDropReports()->clear();
    applyNewConfig(config);
    waitForStateUpdates(getSw());
  };

  verifyAcrossWarmBoots(setup, []() {});
}

uint64_t updateStats(
    AgentEnsemble* ensemble,
    const PortID& portId,
    HwPortStats& prevPortStats) {
  HwPortStats newPortStats = ensemble->getLatestPortStats(portId);
  auto trafficRate = ensemble->getTrafficRate(prevPortStats, newPortStats, 1);
  prevPortStats = newPortStats;
  return trafficRate;
}

TEST_P(AgentMirrorOnDropDnxTest, ModWithMultipleMirrors) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto collectorPortId = portIdsToTest()[0];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  struct TrafficLoop {
    PortID injectionPortId;
    PortID mirrorDestPortId;
    folly::IPAddressV6 dstIp;
  };
  std::vector<TrafficLoop> trafficLoops;
  for (int i = 0;
       i < FLAGS_mod_num_mirrors && (i * 2 + 2) < portIdsToTest().size();
       ++i) {
    auto dstIpArray = kDropDestIp.toByteArray();
    dstIpArray[15] = i;
    trafficLoops.push_back(
        {.injectionPortId = portIdsToTest()[i * 2 + 1],
         .mirrorDestPortId = portIdsToTest()[i * 2 + 2],
         .dstIp = folly::IPAddressV6(dstIpArray)});
    XLOG(DBG3) << "Injection port " << i << ": "
               << portDesc(trafficLoops[i].injectionPortId);
    XLOG(DBG3) << "Mirror destination port " << i << ": "
               << portDesc(trafficLoops[i].mirrorDestPortId);
  }

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {{
            1,
            makeEventConfig(
                cfg::MirrorOnDropAgingGroup::PORT,
                {cfg::MirrorOnDropReasonAggregation::
                     INGRESS_PACKET_PROCESSING_DISCARDS,
                 cfg::MirrorOnDropReasonAggregation::
                     INGRESS_QUEUE_RESOLUTION_DISCARDS}),
        }},
        // Make sure aging is per port and aging interval is minimized (1us)
        {{cfg::MirrorOnDropAgingGroup::PORT, 1 /*agingInterval*/}},
        kLargeTruncateSize);
    for (auto& loop : trafficLoops) {
      addMirror(&config, loop.injectionPortId, loop.mirrorDestPortId);
      addDropPacketAcl(&config, loop.mirrorDestPortId);
    }
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    for (auto& loop : trafficLoops) {
      setupEcmpTraffic(
          loop.injectionPortId,
          loop.dstIp,
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
          true);
    }
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    // Pump traffic to each traffic loop, and get initial stats.
    for (auto& loop : trafficLoops) {
      sendPackets(
          getAgentEnsemble()->getMinPktsForLineRate(loop.injectionPortId),
          loop.injectionPortId,
          loop.dstIp);
    }
    sendPackets(
        getAgentEnsemble()->getMinPktsForLineRate(collectorPortId),
        collectorPortId,
        kCollectorIp_);

    // Verify that traffic loops, mirrors and MOD are all working. For better
    // accuracy, we will calculate the rates for each port separately.
    for (auto& loop : trafficLoops) {
      auto lineRate = getExpectedLineRate(loop.injectionPortId);
      auto portStats = getLatestPortStats(loop.injectionPortId);
      WITH_RETRIES_N(10, {
        uint64_t rate =
            updateStats(getAgentEnsemble(), loop.injectionPortId, portStats);
        XLOGF(INFO, "Port {}: {}/{} bps", loop.injectionPortId, rate, lineRate);
        EXPECT_EVENTUALLY_GE(rate, lineRate);
      });
    }

    for (auto& loop : trafficLoops) {
      auto lineRate = getExpectedLineRate(loop.mirrorDestPortId);
      auto portStats = getLatestPortStats(loop.mirrorDestPortId);
      WITH_RETRIES_N(10, {
        uint64_t rate =
            updateStats(getAgentEnsemble(), loop.mirrorDestPortId, portStats);
        XLOGF(
            INFO, "Port {}: {}/{} bps", loop.mirrorDestPortId, rate, lineRate);
        EXPECT_EVENTUALLY_GE(rate, lineRate);
      });
    }

    auto portStats = getLatestPortStats(collectorPortId);
    WITH_RETRIES_N(10, {
      constexpr uint64_t expectedPPS = 1000000; // 1us aging interval
      constexpr uint64_t packetSize = 450 * 8; // 400B payload + hdrs, in bits
      uint64_t targetRate = expectedPPS * packetSize * trafficLoops.size();
      uint64_t rate =
          updateStats(getAgentEnsemble(), collectorPortId, portStats);
      XLOGF(INFO, "Collector port: {}/{} bps", rate, targetRate);
      // Allow 80% of the expected rate since Eventor throughput is lower.
      EXPECT_EVENTUALLY_GE(rate, targetRate * 0.8);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

// 1. Configures MOD with packet processing drops mapped to event ID
//    `kL3DropEventId`.
// 2. Sends a packet towards an unknown destination.
// 3. Packet will be dropped and a MOD packet will be sent to collector.
// 4. Packets are trapped to CPU. We validate the MOD headers well as a
//    truncated version of the original packet.
TEST_P(AgentMirrorOnDropDnxTest, PacketProcessingDefaultRouteDrop) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto injectionPortId = portIdsToTest()[0];
  auto collectorPortId = portIdsToTest()[1];
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
    waitForStateUpdates(getSw());
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
TEST_P(AgentMirrorOnDropDnxTest, PacketProcessingNullRouteDrop) {
  PortID mirrorPortId = findRecirculationPort(GetParam());
  PortID injectionPortId = portIdsToTest()[0];
  PortID collectorPortId = portIdsToTest()[1];
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
    waitForStateUpdates(getSw());
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
TEST_P(AgentMirrorOnDropDnxTest, PacketProcessingAclDrop) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto injectionPortId = portIdsToTest()[0];
  auto collectorPortId = portIdsToTest()[1];
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
    waitForStateUpdates(getSw());
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
TEST_P(AgentMirrorOnDropDnxTest, MultipleEventIDs) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto injectionPortId = portIdsToTest()[0];
  auto collectorPortId = portIdsToTest()[1];
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
    waitForStateUpdates(getSw());
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
TEST_P(AgentMirrorOnDropDnxTest, VoqReject) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto collectorPortId = portIdsToTest()[0];
  auto ingressPortId = portIdsToTest()[1];
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
    waitForStateUpdates(getSw());
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
TEST_P(AgentMirrorOnDropDnxTest, VsqReject) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto collectorPortId = portIdsToTest()[0];
  auto injectionPortId = portIdsToTest()[1];
  auto txOffPortId = portIdsToTest()[2];
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
    waitForStateUpdates(getSw());
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

TEST_P(AgentMirrorOnDropDnxTest, PrecedenceDrop) {
  auto mirrorPortId = findRecirculationPort(GetParam());
  auto collectorPortId = portIdsToTest()[0];
  auto injectionPortId = portIdsToTest()[1];
  XLOG(DBG3) << "MoD port: " << portDesc(mirrorPortId);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);
  XLOG(DBG3) << "Injection port: " << portDesc(injectionPortId);
  int kPriority = 0;

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();
    setupMirrorOnDrop(
        &config,
        mirrorPortId,
        kCollectorIp_,
        {
            {kPrecedenceDropEventId,
             makeEventConfig(
                 cfg::MirrorOnDropAgingGroup::GLOBAL,
                 {cfg::MirrorOnDropReasonAggregation::
                      UNEXPECTED_REASON_DISCARDS})},
        });
    utility::addTrapPacketAcl(&config, kCollectorNextHopMac_);
    config.switchSettings().ensure().tcToRateLimitKbps().ensure()[kPriority] =
        1048576; // Use rate limit to trigger drops
    applyNewConfig(config);

    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    setupEcmpTraffic(
        injectionPortId,
        kDropDestIp,
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
        true /* disableTtlDecrement */);
    waitForStateUpdates(getSw());
  };

  auto verify = [&]() {
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    auto pkt = sendPackets(1000, injectionPortId, kDropDestIp, kPriority);

    // This test generates two types of dropped packets -- packets dropped
    // from the packet loop, or packets dropped during injection from CPU.
    // We'll verify all packets to make sure both types are accounted for.
    XLOG(DBG3) << "Waiting for mirror packets...";
    int numPackets = 0, numNifPackets = 0;
    while (true) {
      auto frameRx = snooper.waitForPacket(1);
      if (!frameRx.has_value()) {
        break;
      }
      ++numPackets;
      XLOG(DBG3) << "Packet " << numPackets << ":";
      XLOG(DBG3) << PktUtil::hexDump(frameRx->get());
      auto sentBufCopy = pkt->buf()->clone(); // sentBuf will be trimmed
      auto dropType = validateMirrorOnDropPacket(
          frameRx->get(),
          sentBufCopy.get(),
          GetParam(),
          kPrecedenceDropEventId,
          makeSSPA(injectionPortId),
          makePortDSPA(injectionPortId),
          22 /* ignore inner payload hop limit */);
      if (dropType == DroppedPacketType::NIF) {
        numNifPackets++;
      }
    }
    EXPECT_GT(numPackets, 0);
    EXPECT_GT(numNifPackets, 0);
  };

  verifyAcrossWarmBoots(setup, verify);
}

class AgentMirrorOnDropReconfigTest : public AgentMirrorOnDropDnxTest {
 public:
  void setCmdLineFlagOverrides() const override {
    AgentMirrorOnDropDnxTest::setCmdLineFlagOverrides();
    FLAGS_sflow_egress_port_id = modToPortId();
  }

 protected:
  static int32_t modFromPortId() {
    // port id of rcy1/1/445
    return FLAGS_hyper_port ? 32861 : 5;
  }

  static int32_t modToPortId() {
    // port id of rcy1/1/442
    return FLAGS_hyper_port ? 32858 : 2;
  }
};

// The test reconfigures MOD from one recycle port to another recycle port
// while the destination recycle port is congested. The goal is to make sure
// ASIC reset didn't happen and MOD traffic continues to flow.
//
// 1. Create line rate traffic loop on NIF port A and setup a sFlow mirror
//    with 100% sampling, using RCY port 2 as the egress port. This creates
//    a large volume of traffic on RCY port 2.
// 2. Create line rate traffic loop on NIF port B, mirror it to NIF port C,
//    and set a drop ACL on port C.
// 3. Setup MOD to mirror ACL (packet processing) drops to RCY port 1.
// 4. Reconfigure MOD from RCY port 1 to RCY port 2.
//
// In the actual test:
// - NIF port A == masterLogicalInterfacePortIds[1]
// - NIF port B == masterLogicalInterfacePortIds[2]
// - NIF port C == masterLogicalInterfacePortIds[3]
// - MOD collector port == masterLogicalInterfacePortIds[0]
// - RCY port 1 / 2 == modFromPortId() / modToPortId()
//
// Moving MOD to RCY port 2 while it's congested causes some MOD packets to go
// to DRAM. Before CS00012418671 is fixed, this triggers some DRAM related bugs
// with high probability, which results in an ASIC hard reset.
TEST_F(AgentMirrorOnDropReconfigTest, ReconfigUnderTraffic) {
  PortID modFromPort{static_cast<uint16_t>(modFromPortId())};
  PortID modToPort{static_cast<uint16_t>(modToPortId())};
  auto collectorPortId = portIdsToTest()[0];
  XLOG(DBG3) << "MoD port (from): " << portDesc(modFromPort);
  XLOG(DBG3) << "MoD port (to): " << portDesc(modToPort);
  XLOG(DBG3) << "Collector port: " << portDesc(collectorPortId);

  auto loopIp = [&](int i) {
    auto dstIpArray = kDropDestIp.toByteArray();
    dstIpArray[15] = i;
    return folly::IPAddressV6(dstIpArray);
  };

  auto getCurrentRate = [&](const PortID& portId) {
    auto stats1 = getLatestPortStats(portId);
    // NOLINTNEXTLINE(facebook-hte-BadCall-sleep)
    sleep(1);
    auto stats2 = getLatestPortStats(portId);
    return getAgentEnsemble()->getTrafficRate(stats1, stats2, 1);
  };

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();

    std::vector<int> injectionPortIndices;
    std::vector<PortID> mirrorDestPorts;

    // Port 0 is used for the MOD collector.
    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);

    // Ports 1..FLAGS_mod_num_sflow are for mirroring to sFlow.
    std::vector<PortID> sflowSrcPorts;
    configureSflowMirror(config);
    for (int i = 1; i <= FLAGS_mod_num_sflow && i < portIdsToTest().size();
         ++i) {
      PortID injectionPortId = portIdsToTest()[i];
      XLOG(DBG3) << "sFlow source port " << i << ": "
                 << portDesc(injectionPortId);

      setupEcmpTraffic(
          injectionPortId,
          loopIp(i),
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
          true);

      injectionPortIndices.push_back(i); // for verifing traffic rate below
      sflowSrcPorts.push_back(injectionPortId); // for sflow setup below
    }
    utility::configureSflowSampling(config, kSflowMirror, sflowSrcPorts, 1);

    // The next 2 * numPacketDropLoops ports are used for generating
    // line-rate traffic loops and mirroring to a port which drops everything.
    constexpr auto numPacketDropLoops = 1;
    for (int i = 0; i < numPacketDropLoops; ++i) {
      int idx = FLAGS_mod_num_sflow + i * 2 + 1;
      if (idx + 1 >= portIdsToTest().size()) {
        break;
      }
      PortID injectionPortId = portIdsToTest()[idx];
      PortID mirrorDestPortId = portIdsToTest()[idx + 1];
      XLOG(DBG3) << "Injection port " << (i + 1) << ": "
                 << portDesc(injectionPortId);
      XLOG(DBG3) << "Mirror destination port " << (i + 1) << ": "
                 << portDesc(mirrorDestPortId);

      addMirror(&config, injectionPortId, mirrorDestPortId);
      addDropPacketAcl(&config, mirrorDestPortId);
      setupEcmpTraffic(
          injectionPortId,
          loopIp(idx),
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
          true);

      injectionPortIndices.push_back(idx); // for verifying traffic rate below
      mirrorDestPorts.push_back(mirrorDestPortId); // same
    }

    applyNewConfig(config);
    waitForStateUpdates(getSw());

    // Pump traffic into all traffic loops and verify that they are working.
    for (int i : injectionPortIndices) {
      PortID portId = portIdsToTest()[i];
      sendPackets(
          getAgentEnsemble()->getMinPktsForLineRate(portId), portId, loopIp(i));

      WITH_RETRIES_N(10, {
        uint64_t rate = getCurrentRate(portId);
        uint64_t lineRate = getExpectedLineRate(portId);
        XLOGF(INFO, "Port {}: {}/{} bps", portId, rate, lineRate);
        EXPECT_EVENTUALLY_GE(rate, lineRate);
      });
    }

    // Also verify that the mirror dest ports are sending mirror traffic.
    for (const PortID& portId : mirrorDestPorts) {
      WITH_RETRIES_N(10, {
        uint64_t rate = getCurrentRate(portId);
        uint64_t lineRate = getExpectedLineRate(portId);
        XLOGF(INFO, "Port {}: {}/{} bps", portId, rate, lineRate);
        EXPECT_EVENTUALLY_GE(rate, lineRate);
      });
    }
  };

  auto verify = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();

    XLOG(INFO) << "Configuring MOD on port " << modFromPort;
    config.mirrorOnDropReports()->clear();
    setupMirrorOnDrop(&config, modFromPort, kCollectorIp_, prodModConfig());
    applyNewConfig(config);
    waitForStateUpdates(getSw());

    WITH_RETRIES_N(10, {
      uint64_t rate = getCurrentRate(collectorPortId);
      XLOGF(INFO, "Collector port: {} bps", rate);
      EXPECT_EVENTUALLY_GE(rate, 10000000);
    });

    XLOG(INFO) << "Configuring MOD on port " << modToPort;
    config.mirrorOnDropReports()->clear();
    setupMirrorOnDrop(&config, modToPort, kCollectorIp_, prodModConfig());
    applyNewConfig(config);
    waitForStateUpdates(getSw());

    WITH_RETRIES_N(10, {
      uint64_t rate = getCurrentRate(collectorPortId);
      XLOGF(INFO, "Collector port: {} bps", rate);
      EXPECT_EVENTUALLY_GE(rate, 10000000);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

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

// Verify a captured XGS MoD packet against expected values.
// Only checks deterministic fields; timestamps, drop reasons, and
// scheduling metadata are masked (not compared).
void verifyXgsModCapturedPacket(
    const XgsModPacketParsed& captured,
    const folly::MacAddress& expectedDstMac,
    const folly::IPAddressV6& expectedSrcIp,
    const folly::IPAddressV6& expectedDstIp,
    uint16_t expectedSrcPort,
    uint16_t expectedDstPort,
    uint32_t expectedObservationDomainId,
    uint32_t expectedSwitchId,
    uint16_t expectedIngressPort,
    const folly::IPAddressV6& expectedInnerSrcIp,
    const folly::IPAddressV6& expectedInnerDstIp,
    uint16_t expectedInnerSrcPort,
    uint16_t expectedInnerDstPort) {
  // Outer Ethernet - only check dst MAC; src MAC is the physical port MAC
  // assigned by the ASIC, which is not known at test configuration time.
  EXPECT_EQ(captured.ethHeader.getDstMac(), expectedDstMac);

  // IPv6 - compare deterministic fields only (skip HopLimit, PayloadLength)
  EXPECT_EQ(captured.ipv6Header.srcAddr, expectedSrcIp);
  EXPECT_EQ(captured.ipv6Header.dstAddr, expectedDstIp);
  EXPECT_EQ(
      captured.ipv6Header.nextHeader,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));

  // UDP - compare ports and checksum (always 0 for PSAMP)
  EXPECT_EQ(captured.udpHeader.srcPort, expectedSrcPort);
  EXPECT_EQ(captured.udpHeader.dstPort, expectedDstPort);
  EXPECT_EQ(captured.udpHeader.csum, 0);

  // IPFIX header
  EXPECT_EQ(captured.psampPacket.ipfixHeader.version, psamp::IPFIX_VERSION);
  EXPECT_EQ(
      captured.psampPacket.ipfixHeader.observationDomainId,
      expectedObservationDomainId);

  // PSAMP template
  EXPECT_EQ(
      captured.psampPacket.templateHeader.templateId,
      psamp::XGS_PSAMP_TEMPLATE_ID);

  // PSAMP data - deterministic fields
  EXPECT_EQ(captured.psampPacket.data.switchId, expectedSwitchId);
  EXPECT_EQ(captured.psampPacket.data.ingressPort, expectedIngressPort);
  EXPECT_EQ(
      captured.psampPacket.data.varLenIndicator,
      psamp::XGS_PSAMP_VAR_LEN_INDICATOR);

  // Parse the inner packet from sampledPacketData to verify its IPv6/UDP
  // header fields. The inner packet is a truncated copy of the original
  // dropped packet. MACs may be rewritten and VLAN tags stripped by the
  // ASIC, so we skip the Ethernet header and compare from IPv6 onward.
  auto innerBuf = folly::IOBuf::copyBuffer(
      captured.psampPacket.data.sampledPacketData.data(),
      captured.psampPacket.data.sampledPacketData.size());
  folly::io::Cursor innerCursor(innerBuf.get());

  // Skip inner Ethernet header (no VLAN tag in sampled data): 14 bytes
  innerCursor.skip(14);

  IPv6Hdr innerIpv6(innerCursor);
  EXPECT_EQ(innerIpv6.srcAddr, expectedInnerSrcIp);
  EXPECT_EQ(innerIpv6.dstAddr, expectedInnerDstIp);
  EXPECT_EQ(innerIpv6.nextHeader, static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP));

  UDPHeader innerUdp;
  innerUdp.parse(&innerCursor);
  EXPECT_EQ(innerUdp.srcPort, expectedInnerSrcPort);
  EXPECT_EQ(innerUdp.dstPort, expectedInnerDstPort);
}

class AgentMirrorOnDropXgsTest : public AgentMirrorOnDropTest {
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
      if (std::all_of(ports.begin(), ports.end(), [&](auto portId) {
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
  cfg::MirrorTunnel tunnel;
  tunnel.srcIp() = switchIp.str();
  cfg::MirrorDestination mirrorDest;
  mirrorDest.tunnel() = tunnel;
  report.mirrorPort() = mirrorDest;
  return report;
}

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

        verifyXgsModCapturedPacket(
            parsed,
            kCollectorNextHopMac_,
            kSwitchIp_,
            kCollectorIp_,
            kMirrorSrcPort,
            kMirrorDstPort,
            1, // observationDomainId
            0, // switchId
            static_cast<uint16_t>(injectionPortId),
            kPacketSrcIp_,
            kDropDestIp,
            kPacketSrcPort,
            kPacketDstPort);
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
    auto hwAsic = checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
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
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState()));
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

        verifyXgsModCapturedPacket(
            parsed,
            kCollectorNextHopMac_,
            kSwitchIp_,
            kCollectorIp_,
            kMirrorSrcPort,
            kMirrorDstPort,
            1, // observationDomainId
            0, // switchId
            static_cast<uint16_t>(injectionPortId),
            kPacketSrcIp_,
            kDropDestIp,
            kPacketSrcPort,
            kPacketDstPort);
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

        verifyXgsModCapturedPacket(
            parsed,
            kCollectorNextHopMac_,
            kSwitchIp_,
            kCollectorIp_,
            kMirrorSrcPort,
            kMirrorDstPort,
            1, // observationDomainId
            0, // switchId
            static_cast<uint16_t>(injectionPortId),
            kPacketSrcIp_,
            kAclDropDestIp,
            kPacketSrcPort,
            kPacketDstPort);
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
      utility::getMacForFirstInterfaceWithPorts(getProgrammedState()).u64HBO() +
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
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState()),
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
        getAgentEnsemble()->getMinPktsForLineRate(trafficPortId),
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
} // namespace facebook::fboss
