// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/IPAddressV4.h>
#include <type_traits>
#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/packet/UDPDatagram.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/MultiPortTrafficTestUtils.h"
#include "fboss/agent/test/utils/NetworkAITestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PfcTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/SflowShimUtils.h"

DEFINE_int32(sflow_test_rate, 90000, "sflow sampling rate for hw test");
constexpr uint16_t kTimeoutSecs = 1;

const std::string kSflowMirror = "sflow_mirror";

namespace facebook::fboss {

template <typename AddrT>
class AgentSflowMirrorTest : public AgentHwTest {
 public:
  // Index in the sample ports where data traffic is expected!
  const int kDataTrafficPortIndex{0};
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {ProductionFeature::SFLOWv4_SAMPLING};
    } else {
      return {ProductionFeature::SFLOWv6_SAMPLING};
    }
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);

    auto port0 = ensemble.masterLogicalInterfacePortIds()[0];
    auto port0Switch =
        ensemble.getSw()->getScopeResolver()->scope(port0).switchId();
    auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(port0Switch);
    auto ports =
        getPortsForSampling(ensemble.masterLogicalInterfacePortIds(), asic);
    if (asic->isSupported(HwAsic::Feature::EVENTOR_PORT_FOR_SFLOW)) {
      utility::addEventorVoqConfig(&cfg, cfg::StreamType::UNICAST);
      for (auto& port : *cfg.ports()) {
        if (port.portType() == cfg::PortType::EVENTOR_PORT) {
          port.maxFrameSize() = 1600; // Guard against large packets
        }
      }
    }
    utility::setTTLZeroCpuConfig(ensemble.getL3Asics(), cfg);
    return cfg;
  }

  virtual bool enableTruncation() const = 0;

  void configureMirror(
      cfg::SwitchConfig& cfg,
      std::optional<bool> isV4Addr = std::nullopt,
      uint32_t udpSrcPort = 6545,
      uint32_t udpDstPort = 6343,
      std::optional<int> sampleRate = std::nullopt) const {
    bool v4 = isV4Addr.has_value() ? *isV4Addr
                                   : std::is_same_v<AddrT, folly::IPAddressV4>;
    utility::configureSflowMirror(
        cfg,
        kSflowMirror,
        enableTruncation(),
        utility::getSflowMirrorDestination(v4).str(),
        udpSrcPort,
        udpDstPort,
        v4,
        sampleRate);
  }

  void configSampling(cfg::SwitchConfig& config, int sampleRate) const {
    auto ports = getPortsForSampling();
    utility::configureSflowSampling(config, kSflowMirror, ports, sampleRate);
  }

  void configureMirrorWithSampling(
      cfg::SwitchConfig& cfg,
      int sampleRate,
      std::optional<bool> v4 = std::nullopt,
      uint32_t udpSrcPort = 6545,
      uint32_t udpDstPort = 6343) const {
    std::optional<int> sampleRateCfg;
    auto asic = checkSameAndGetAsic();
    if (asic->isSupported(HwAsic::Feature::SAMPLE_RATE_CONFIG_PER_MIRROR)) {
      // Handle sampleRate config needed on mirror
      sampleRateCfg = sampleRate;
    }
    configureMirror(cfg, v4, udpSrcPort, udpDstPort, sampleRateCfg);
    configSampling(cfg, sampleRate);
  }

  void configureTrapAcl(
      cfg::SwitchConfig& cfg,
      bool isV4 = std::is_same_v<AddrT, folly::IPAddressV4>) const {
    auto asic = checkSameAndGetAsic();
    return asic->isSupported(HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER)
        ? utility::configureTrapAcl(
              asic, cfg, getNonSflowSampledInterfacePort())
        : utility::configureTrapAcl(asic, cfg, isV4);
  }

  PortID getNonSflowSampledInterfacePort() const {
    return checkSameAndGetAsic()->isSupported(HwAsic::Feature::MANAGEMENT_PORT)
        ? masterLogicalPortIds({cfg::PortType::MANAGEMENT_PORT})[0]
        : masterLogicalInterfacePortIds()[0];
  }

  cfg::PortType getNonSflowSampledInterfacePortType() const {
    return checkSameAndGetAsic()->isSupported(HwAsic::Feature::MANAGEMENT_PORT)
        ? cfg::PortType::MANAGEMENT_PORT
        : cfg::PortType::INTERFACE_PORT;
  }

  std::vector<PortID> getPortsForSampling() const {
    auto allIntfPorts = masterLogicalInterfacePortIds();
    auto nonSampledPort = getNonSflowSampledInterfacePort();
    // Ports with sampling enabled are all interface ports except any
    // on which we are sending the sampled packets out!
    std::vector<PortID> sampledPorts;
    std::copy_if(
        allIntfPorts.begin(),
        allIntfPorts.end(),
        std::back_inserter(sampledPorts),
        [nonSampledPort](const auto& portId) {
          return portId != nonSampledPort;
        });
    auto switchID = switchIdForPort(allIntfPorts[0]);
    auto asic = getSw()->getHwAsicTable()->getHwAsic(switchID);
    return getPortsForSampling(sampledPorts, asic);
  }

  std::vector<PortID> getPortsForSampling(
      const std::vector<PortID>& ports,
      const HwAsic* asic) const {
    /*
     * Tajo does not share sample packet session yet and creates
     * one per port and the maximum is 16. Configure 16 ports
     * for now on Tajo to enable sflow with mirroring.
     */
    if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO) {
      return std::vector<PortID>(ports.begin(), ports.begin() + 16);
    }
    return ports;
  }

  const HwAsic* checkSameAndGetAsic() const {
    return facebook::fboss::checkSameAndGetAsic(
        getAgentEnsemble()->getL3Asics());
  }

  std::optional<uint32_t> getHwLogicalPortId(PortID port) const {
    auto asic = checkSameAndGetAsic();
    if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO ||
        asic->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA) {
      return std::nullopt;
    }
    return getSw()->getHwLogicalPortId(port);
  }

  void verifySflowExporterIp(const std::vector<uint8_t>& sflowPayload) {
    auto asic = checkSameAndGetAsic();
    if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      uint8_t exporterIpBytes[16];
      for (int i = 0; i < 16; i++) {
        exporterIpBytes[i] = sflowPayload[i + 8];
      }
      auto exporterIp =
          folly::IPAddressV6::fromBinary(folly::ByteRange(exporterIpBytes, 16));
      EXPECT_EQ(exporterIp, utility::getSflowMirrorSource(false /* isV4 */));
    }
  }
  void validateSflowPacketHeader(
      const std::vector<uint8_t>& sflowPayload,
      PortID srcPortId) {
    PortID expectedSrcPortId;
    /*
     * sflow shim format for Tajo:
     *
     * Source system port GID - 16 bits
     * Destination system port GID - 16 bits
     * Source logical port GID - 20 bits
     * Destination logical port GID - 20 bits
     *
     * sflow shim format for BCM:
     *
     * version field (if applicable): 32, sport : 8, smod : 8, dport : 8,
     * dmod : 8, source_sample : 1, dest_sample : 1, flex_sample : 1
     * multicast : 1, discarded : 1, truncated : 1,
     * dest_port_encoding : 3, reserved : 23
     */
    auto asic = checkSameAndGetAsic();
    if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO ||
        asic->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA) {
      auto systemPortId = sflowPayload[0] << 8 | sflowPayload[1];
      expectedSrcPortId =
          static_cast<PortID>(systemPortId - asic->getSflowPortIDOffset());
    } else if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
      /*
       * Bytes 68 through 71 carry the ingress ifindex in sflow v5 header
       * which is a 32 bit field
       */
      auto constexpr offsetBytesStart = 68;
      auto systemPortId = 0;
      for (int i = 0, j = 24; i < 4; i++, j -= 8) {
        systemPortId |=
            static_cast<uint32_t>((sflowPayload[offsetBytesStart + i]) << j);
      }
      expectedSrcPortId =
          getPortID(SystemPortID(systemPortId), getProgrammedState());
      verifySflowExporterIp(sflowPayload);
    } else {
      auto sourcePortOffset = 0;
      if (asic->isSupported(HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD)) {
        sourcePortOffset += 4;
      }
      expectedSrcPortId = static_cast<PortID>(sflowPayload[sourcePortOffset]);
    }
    EXPECT_EQ(expectedSrcPortId, srcPortId);
  }

  uint16_t getMirrorTruncateSize() const {
    return checkSameAndGetAsic()->getMirrorTruncateSize();
  }

  template <typename T = AddrT>
  void resolveRouteForMirrorDestinationImpl() {
    const auto mirrorDestinationPort = getNonSflowSampledInterfacePort();
    boost::container::flat_set<PortDescriptor> nhopPorts{
        PortDescriptor(mirrorDestinationPort)};

    this->getAgentEnsemble()->applyNewState(
        [&](const std::shared_ptr<SwitchState>& state) {
          utility::EcmpSetupTargetedPorts<T> ecmpHelper(
              state,
              getSw()->needL2EntryForNeighbor(),
              RouterID(0),
              {getNonSflowSampledInterfacePortType()});
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
        {getNonSflowSampledInterfacePortType()});

    ecmpHelper.programRoutes(
        getAgentEnsemble()->getRouteUpdaterWrapper(), nhopPorts, {prefix});
  }

  void resolveRouteForMirrorDestination(
      bool isV4 = std::is_same_v<AddrT, folly::IPAddressV4>) {
    if (isV4) {
      resolveRouteForMirrorDestinationImpl<folly::IPAddressV4>();
    } else {
      resolveRouteForMirrorDestinationImpl<folly::IPAddressV6>();
    }
  }

  std::string getTrafficDestinationIp(int portIndex) {
    return folly::to<std::string>(kIpStr, portIndex, "::1");
  }

  void setupEcmpTraffic() {
    utility::EcmpSetupTargetedPorts6 ecmpHelper{
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState())};

    const PortDescriptor port(getDataTrafficPort());
    RoutePrefixV6 route{
        folly::IPAddressV6(getTrafficDestinationIp(kDataTrafficPortIndex)),
        128};

    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, {port});
    });

    auto routeUpdater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&routeUpdater, {port}, {route});

    utility::ttlDecrementHandlingForLoopbackTraffic(
        getAgentEnsemble(), ecmpHelper.getRouterId(), ecmpHelper.nhop(port));
  }

  std::unique_ptr<facebook::fboss::TxPacket> genPacket(
      int portIndex,
      size_t payloadSize) {
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    folly::IPAddressV6 sip{"2401:db00:dead:beef::2401"};
    folly::IPAddressV6 dip{getTrafficDestinationIp(portIndex)};
    uint16_t sport = 9701;
    uint16_t dport = 9801;
    std::vector<uint8_t> payload(payloadSize, 0xff);
    auto pkt = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        intfMac,
        intfMac,
        sip,
        dip,
        sport,
        dport,
        0,
        255,
        payload);
    return pkt;
  }

  int getSflowPacketNonSflowHeaderLen(bool isV4) {
    auto ipHeaderLen = isV4 ? 20 : 40;
    auto asic = checkSameAndGetAsic();
    auto vlanHeaderLen = asic->getSwitchType() == cfg::SwitchType::VOQ ? 0 : 4;
    return 14 /* ethernet header */ + vlanHeaderLen + ipHeaderLen +
        UDPHeader::size();
  }

  int getSflowPacketHeaderLength(bool isV4) {
    auto asic = checkSameAndGetAsic();
    int sflowShimHeaderLength = asic->getSflowShimHeaderSize();
    if (asic->isSupported(HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD)) {
      sflowShimHeaderLength += 4;
    }
    return sflowShimHeaderLength + getSflowPacketNonSflowHeaderLen(isV4);
  }

  void verifySampledPacket(
      bool isV4 = std::is_same_v<AddrT, folly::IPAddressV4>) {
    auto ports = getPortsForSampling();
    getAgentEnsemble()->bringDownPorts(
        std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 256);
    auto length = pkt->buf()->length();

    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    XLOG(DBG2) << "Sending packet through port " << ports[1];
    getAgentEnsemble()->sendPacketAsync(
        std::move(pkt), PortDescriptor(ports[1]), std::nullopt);

    std::optional<std::unique_ptr<folly::IOBuf>> capturedPktBuf;
    WITH_RETRIES({
      capturedPktBuf = snooper.waitForPacket(kTimeoutSecs);
      EXPECT_EVENTUALLY_TRUE(capturedPktBuf.has_value());
    });
    folly::io::Cursor capturedPktCursor{capturedPktBuf->get()};
    auto capturedPkt = utility::EthFrame(capturedPktCursor);

    // captured packet has encap header on top
    ASSERT_GE(capturedPkt.length(), length);
    EXPECT_GE(capturedPkt.length(), getSflowPacketHeaderLength(isV4));

    auto payloadLength = length;
    if (checkSameAndGetAsic()->getAsicType() ==
        cfg::AsicType::ASIC_TYPE_JERICHO3) {
      /*
       * J3 pads upto 512 bytes if the packet size is < 512 bytes.
       * J3 trimes upto 512 bytes if the packet size is > 512 bytes.
       */
      payloadLength = 512;
    }
    auto delta = capturedPkt.length() - payloadLength;
    EXPECT_LE(delta, getSflowPacketHeaderLength(isV4));
    auto udpPayload = isV4 ? capturedPkt.v4PayLoad()->udpPayload()
                           : capturedPkt.v6PayLoad()->udpPayload();
    auto payload = udpPayload->payload();
    EXPECT_EQ(udpPayload->header().csum, 0);
    auto hwLogicalPortId = getHwLogicalPortId(ports[1]);
    if (!hwLogicalPortId) {
      validateSflowPacketHeader(payload, ports[1]);
    } else {
      validateSflowPacketHeader(payload, PortID(*hwLogicalPortId));
    }

    if (checkSameAndGetAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO &&
        checkSameAndGetAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_YUBA &&
        checkSameAndGetAsic()->getAsicType() !=
            cfg::AsicType::ASIC_TYPE_JERICHO3) {
      // Call parseSflowShim here. And verify
      // 1. Src port is correct
      // 2. Parser correctly identified whether the pkt is from TH3 or TH4.
      // TODO(vsp): As we incorporate Cisco sFlow packet in parser, enhance this
      // test case.
      auto buf = folly::IOBuf::wrapBuffer(payload.data(), payload.size());
      folly::io::Cursor cursor{buf.get()};
      auto shim = utility::parseSflowShim(cursor);
      XLOG(DBG2) << fmt::format(
          "srcPort = {}, dstPort = {}", shim.srcPort, shim.dstPort);
      if (!hwLogicalPortId) {
        EXPECT_EQ(
            shim.srcPort, static_cast<uint32_t>(getPortsForSampling()[1]));
      } else {
        EXPECT_EQ(shim.srcPort, static_cast<uint32_t>(*hwLogicalPortId));
      }
      if (checkSameAndGetAsic()->isSupported(
              HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD)) {
        EXPECT_EQ(shim.asic, utility::SflowShimAsic::SFLOW_SHIM_ASIC_TH4);
      } else {
        EXPECT_EQ(shim.asic, utility::SflowShimAsic::SFLOW_SHIM_ASIC_TH3);
      }
    }
  }

  void verifySflowPacketUdpChecksum(
      const folly::IOBuf* capturedPktBuf,
      const utility::EthFrame& capturedPkt,
      const std::optional<utility::UDPDatagram>& udpPayload,
      bool isV4) {
    uint16_t csum = 0;
    if (udpPayload->header().csum) {
      folly::io::Cursor pktCursor{capturedPktBuf};
      pktCursor += getSflowPacketNonSflowHeaderLen(isV4);
      auto udpHdr = UDPHeader(
          udpPayload->header().srcPort,
          udpPayload->header().dstPort,
          udpPayload->header().length,
          udpPayload->header().csum);
      if (isV4) {
        csum = udpHdr.computeChecksum(
            capturedPkt.v4PayLoad()->header(), pktCursor);
      } else {
        csum = udpHdr.computeChecksum(
            capturedPkt.v6PayLoad()->header(), pktCursor);
      }
    }
    XLOG(DBG2) << "Sflow packet expected UDP csum: " << csum
               << ", csum in packet: " << udpPayload->header().csum;
    // If sflow packet has non zero UDP csum, then make sure that
    // its as expected!
    EXPECT_EQ(udpPayload->header().csum, csum);
  }

  void verifySampledPacketWithTruncate(
      bool isV4 = std::is_same_v<AddrT, folly::IPAddressV4>) {
    auto ports = getPortsForSampling();
    getAgentEnsemble()->bringDownPorts(
        std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 8000);
    auto length = pkt->buf()->length();

    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    XLOG(DBG2) << "Sending packet through port " << ports[1];
    getAgentEnsemble()->sendPacketAsync(
        std::move(pkt), PortDescriptor(ports[1]), std::nullopt);

    std::optional<std::unique_ptr<folly::IOBuf>> capturedPktBuf;
    WITH_RETRIES({
      capturedPktBuf = snooper.waitForPacket(kTimeoutSecs);
      EXPECT_EVENTUALLY_TRUE(capturedPktBuf.has_value());
    });
    folly::io::Cursor capturedPktCursor{capturedPktBuf->get()};
    auto capturedPkt = utility::EthFrame(capturedPktCursor);

    // captured packet has encap header on top
    EXPECT_LE(capturedPkt.length(), length);
    auto capturedHdrSize = getSflowPacketHeaderLength(isV4);
    EXPECT_GE(capturedPkt.length(), capturedHdrSize);

    EXPECT_LE(
        capturedPkt.length() - capturedHdrSize,
        getMirrorTruncateSize()); /* TODO: confirm length in CS00010399535 and
                                     CS00012130950 */
    auto udpPayload = isV4 ? capturedPkt.v4PayLoad()->udpPayload()
                           : capturedPkt.v6PayLoad()->udpPayload();
    verifySflowExporterIp(udpPayload->payload());
    verifySflowPacketUdpChecksum(
        capturedPktBuf->get(), capturedPkt, udpPayload, isV4);
  }

  uint64_t getSampleCount(const std::map<PortID, HwPortStats>& stats) {
    const auto& portStats = stats.at(getNonSflowSampledInterfacePort());
    return *portStats.outUnicastPkts_();
  }

  const PortID getDataTrafficPort() {
    return getPortsForSampling()[kDataTrafficPortIndex];
  }

  uint64_t getExpectedSampleCount(const std::map<PortID, HwPortStats>& stats) {
    uint64_t expectedSampleCount = 0;
    auto port = getDataTrafficPort();
    const auto& portStats = stats.at(port);
    // In theory it's more correct to calcuate this based on inUnicastPkts, but
    // Rx counters are not supported on TH5 in EDB loopback, so use Tx.
    expectedSampleCount =
        (*portStats.outUnicastPkts_() / FLAGS_sflow_test_rate);
    XLOG(DBG2) << "total packets tx " << *portStats.outUnicastPkts_();
    return expectedSampleCount;
  }

  void verifySampledPacketRate() {
    auto trafficPort = getDataTrafficPort();
    for (int i = 0; i < getAgentEnsemble()->getMinPktsForLineRate(trafficPort);
         i++) {
      auto pkt = genPacket(kDataTrafficPortIndex, 256 /*payloadSize*/);
      getAgentEnsemble()->sendPacketAsync(
          std::move(pkt), PortDescriptor(trafficPort), std::nullopt);
    }
    const uint64_t portSpeedBps =
        static_cast<uint64_t>(getProgrammedState()
                                  ->getPorts()
                                  ->getNodeIf(trafficPort)
                                  ->getSpeed()) *
        1000 * 1000 * 0.99; // 99% is good enough
    getAgentEnsemble()->waitForSpecificRateOnPort(trafficPort, portSpeedBps);

    auto ports = getPortsForSampling();
    getAgentEnsemble()->bringDownPorts(
        std::vector<PortID>(ports.begin() + 1, ports.end()));
    ports.push_back(getNonSflowSampledInterfacePort());
    int percentError = 100;
    WITH_RETRIES({
      auto stats = getLatestPortStats(ports);
      auto actualSampleCount = getSampleCount(stats);
      auto expectedSampleCount = getExpectedSampleCount(stats);
      XLOG(DBG2) << "Number of sflow samples expected: " << expectedSampleCount
                 << ", samples seen: " << actualSampleCount;
      auto difference = (expectedSampleCount > actualSampleCount)
          ? (expectedSampleCount - actualSampleCount)
          : (actualSampleCount - expectedSampleCount);
      if (actualSampleCount) {
        percentError = (difference * 100) / actualSampleCount;
      }
      EXPECT_EVENTUALLY_LE(percentError, kDefaultPercentErrorThreshold);
    });

    getAgentEnsemble()->bringUpPorts(
        std::vector<PortID>(ports.begin() + 1, ports.end()));
    getAgentEnsemble()->clearPortStats();
  }

  virtual void testSampledPacket() {
    auto setup = [=, this]() {
      auto ports = getPortsForSampling();
      auto config = initialConfig(*getAgentEnsemble());
      configureMirrorWithSampling(config, 1 /*sampleRate*/);
      configureTrapAcl(config);
      applyNewConfig(config);
      resolveRouteForMirrorDestination();
    };
    auto verify = [=, this]() {
      if (enableTruncation()) {
        verifySampledPacketWithTruncate();
      } else {
        verifySampledPacket();
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void testSampledPacketRate() {
    auto setup = [=, this]() {
      auto config = initialConfig(*getAgentEnsemble());
      configureMirrorWithSampling(config, FLAGS_sflow_test_rate /*sampleRate*/);
      applyNewConfig(config);
      resolveRouteForMirrorDestination();
      setupEcmpTraffic();
    };
    auto verify = [=, this]() { verifySampledPacketRate(); };
    verifyAcrossWarmBoots(setup, verify);
  }

  void stressRecreateMirror() {
    auto setup = [=, this]() {
      auto ports = getPortsForSampling();

      // Test for resource leaks by repeatedly creating and removing mirror.
      for (int i = 0; i < 50; i++) {
        XLOG(INFO) << "Add mirror iteration " << i;
        auto config = initialConfig(*getAgentEnsemble());
        configureMirrorWithSampling(config, 1 /*sampleRate*/);
        applyNewConfig(config);
        resolveRouteForMirrorDestination();

        XLOG(INFO) << "Remove mirror iteration " << i;
        config = initialConfig(*getAgentEnsemble()); // remove mirror
        applyNewConfig(config);
      }

      XLOG(INFO) << "Final add mirror";
      auto config = initialConfig(*getAgentEnsemble());
      configureMirrorWithSampling(config, 1 /*sampleRate*/);
      configureTrapAcl(config);
      applyNewConfig(config);
      resolveRouteForMirrorDestination();
    };
    auto verify = [=, this]() {
      if (enableTruncation()) {
        verifySampledPacketWithTruncate();
      } else {
        verifySampledPacket();
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void verifySrcPortRandomizationOnSflowPacket(uint16_t numPackets = 10) {
    auto ports = getPortsForSampling();
    getAgentEnsemble()->bringDownPorts(
        std::vector<PortID>(ports.begin() + 2, ports.end()));
    std::set<uint16_t> l4SrcPorts;
    for (auto i = 0; i < numPackets; i++) {
      auto pkt = genPacket(1, 256);
      utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
      XLOG(DBG2) << "Sending packet through port " << ports[1];
      getAgentEnsemble()->sendPacketAsync(
          std::move(pkt), PortDescriptor(ports[1]), std::nullopt);

      std::optional<std::unique_ptr<folly::IOBuf>> capturedPktBuf;
      WITH_RETRIES({
        capturedPktBuf = snooper.waitForPacket(kTimeoutSecs);
        EXPECT_EVENTUALLY_TRUE(capturedPktBuf.has_value());
      });
      folly::io::Cursor capturedPktCursor{capturedPktBuf->get()};
      auto capturedPkt = utility::EthFrame(capturedPktCursor);
      auto udpPayload = capturedPkt.v6PayLoad()->udpPayload();
      // Validate UDP checksum added to the packet is accurate
      verifySflowPacketUdpChecksum(
          capturedPktBuf->get(), capturedPkt, udpPayload, false /*isV4*/);
      l4SrcPorts.insert(udpPayload->header().srcPort);
      // J3 randomize the source based on the time stamp in the asic.
      // Add a delay to ensure the source ports are different for
      // every packet that is trapped to CPU with SFLOW header.
      sleep(2);
    }
    EXPECT_GT(l4SrcPorts.size(), 1);
  }

  utility::MacAddressGenerator macGenerator = utility::MacAddressGenerator();
  constexpr static size_t kDefaultPayloadSize = 1400;
  constexpr static size_t kDefaultPercentErrorThreshold = 7;
  constexpr static auto kIpStr = "2401:db00:dead:beef:";
};

template <typename AddrT>
class AgentSflowMirrorUntruncateTest : public AgentSflowMirrorTest<AddrT> {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {
          ProductionFeature::SFLOWv4_SAMPLING,
          ProductionFeature::UNTRUNCATED_SFLOW};
    } else {
      return {
          ProductionFeature::SFLOWv6_SAMPLING,
          ProductionFeature::UNTRUNCATED_SFLOW};
    }
  }

  virtual bool enableTruncation() const override {
    return false;
  }
};

template <typename AddrT>
class AgentSflowMirrorTruncateTest : public AgentSflowMirrorTest<AddrT> {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {
          ProductionFeature::SFLOWv4_SAMPLING,
          ProductionFeature::MIRROR_PACKET_TRUNCATION};
    } else {
      return {
          ProductionFeature::SFLOWv6_SAMPLING,
          ProductionFeature::MIRROR_PACKET_TRUNCATION};
    }
  }

  virtual bool enableTruncation() const override {
    return true;
  }
};

// These tests try to program sflow mirror with IPv4, then change the config
// to IPv6, or vice versa. So both V4 and V6 must be supported.
class AgentSflowMirrorAddressFamilySwitchingTest
    : public AgentSflowMirrorTruncateTest<folly::IPAddressV4> {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::SFLOWv4_SAMPLING,
        ProductionFeature::SFLOWv6_SAMPLING,
        ProductionFeature::MIRROR_PACKET_TRUNCATION};
  }
};

template <typename AddrT>
class AgentSflowMirrorOnTrunkTest : public AgentSflowMirrorTruncateTest<AddrT> {
 public:
  using AgentSflowMirrorTest<AddrT>::getPortsForSampling;

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {
          ProductionFeature::SFLOWv4_SAMPLING,
          ProductionFeature::LAG,
          ProductionFeature::MIRROR_PACKET_TRUNCATION,
          ProductionFeature::LAG_MIRRORING};
    } else {
      return {
          ProductionFeature::SFLOWv6_SAMPLING,
          ProductionFeature::LAG,
          ProductionFeature::MIRROR_PACKET_TRUNCATION,
          ProductionFeature::LAG_MIRRORING};
    }
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentSflowMirrorTruncateTest<AddrT>::initialConfig(ensemble);
    configTrunk(config, ensemble);
    return config;
  }

  void configTrunk(cfg::SwitchConfig& config, const AgentEnsemble& ensemble)
      const {
    auto port0 = ensemble.masterLogicalPortIds()[0];
    auto port0Switch =
        ensemble.getSw()->getScopeResolver()->scope(port0).switchId();
    auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(port0Switch);
    utility::addAggPort(
        1,
        {getPortsForSampling(ensemble.masterLogicalPortIds(), asic)[0]},
        &config);
  }
};

class AgentSflowMirrorWithLineRateTrafficTest
    : public AgentSflowMirrorTruncateTest<folly::IPAddressV6> {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    auto productionFeatures = AgentSflowMirrorTruncateTest<
        folly::IPAddressV6>::getProductionFeaturesVerified();
    productionFeatures.push_back(ProductionFeature::LINERATE_SFLOW);
    return productionFeatures;
  }

  void setCmdLineFlagOverrides() const override {
    AgentSflowMirrorTruncateTest::setCmdLineFlagOverrides();
    // VerifySflowEgressCongestionShort is also used in n-warmboot tests, where
    // we want to test basic fabric port init.
    FLAGS_hide_fabric_ports = false;
    // Test that neighbor advertisements or stray packets won't cause eventor
    // port to get stuck (CS00012404377).
    FLAGS_disable_neighbor_updates = false;
    FLAGS_allow_eventor_send_packet = true;
  }

  static const int kLosslessPriority{2};
  void testSflowEgressCongestion(int iterations) {
    constexpr int kNumDataTrafficPorts{6};
    auto setup = [=, this]() {
      auto allPorts = masterLogicalInterfacePortIds();
      std::vector<PortID> portIds(
          allPorts.begin(), allPorts.begin() + kNumDataTrafficPorts);
      std::vector<int> losslessPgIds = {kLosslessPriority};
      std::vector<int> lossyPgIds = {0};
      auto config = initialConfig(*getAgentEnsemble());
      // Configure 1:1 sampling to ensure high rate on mirror egress port
      configureMirrorWithSampling(config, 1 /*sampleRate*/);
      // PFC buffer configurations to ensure we have lossless traffic
      const std::map<int, int> tcToPgOverride{};
      // We dont want PFC here, so set global shared threshold to be high
      auto asicType =
          checkSameAndGetAsicType(this->initialConfig(*getAgentEnsemble()));
      auto bufferParams =
          utility::PfcBufferParams::getPfcBufferParams(asicType);
      bufferParams.globalShared = 20 * 1024 * 1024;
      bufferParams.scalingFactor = cfg::MMUScalingFactor::ONE;
      utility::setupPfcBuffers(
          getAgentEnsemble(),
          config,
          portIds,
          losslessPgIds,
          lossyPgIds,
          tcToPgOverride,
          bufferParams);
      // Make sure that traffic is going to loop for ever!
      utility::setTTLZeroCpuConfig(getAgentEnsemble()->getL3Asics(), config);
      applyNewConfig(config);
      resolveRouteForMirrorDestination();
      utility::setupEcmpDataplaneLoopOnAllPorts(getAgentEnsemble());
      // 1. Low rate of the order of 1% of 800G is good enough to saturate the
      // eventor port, which currently has a max possible rate of ~13Gbps.
      // 2. For prod, we sample 1 out of 60K packets, hence the rate expected
      // on eventor port is low, 1/60K of 14.4Tbps ~240Mbps.
      utility::createTrafficOnMultiplePorts(
          getAgentEnsemble(),
          kNumDataTrafficPorts,
          sendPacket,
          1 /*desiredPctLineRate*/);
    };
    auto verify = [=, this]() {
      // Attempt to send packets directly into the eventor port to brick it.
      if (checkSameAndGetAsic()->isSupported(
              HwAsic::Feature::EVENTOR_PORT_FOR_SFLOW)) {
        auto eventorPortId =
            masterLogicalPortIds({cfg::PortType::EVENTOR_PORT})[0];
        XLOG(INFO) << "Eventor port: " << eventorPortId;
        for (int i = 0; i < 10; ++i) {
          auto pkt = utility::makeUDPTxPacket(
              getSw(),
              std::nullopt, // vlanID
              folly::MacAddress("02:00:00:00:0F:0B"),
              folly::MacAddress("FF:FF:FF:FF:FF:FF"),
              folly::IPAddressV6("::1"),
              folly::IPAddressV6("::2"),
              1234, // srcPort
              5678, // dstPort
              0, // dscp
              255, // hopLimit
              std::vector<uint8_t>(100, 0xff)); // 100B payload
          getAgentEnsemble()->sendPacketAsync(
              std::move(pkt), PortDescriptor(eventorPortId), std::nullopt);
        }
      }

      verifySflowEgressPortNotStuck(iterations);
      if (checkSameAndGetAsic()->isSupported(
              HwAsic::Feature::EVENTOR_PORT_FOR_SFLOW)) {
        validateEventorPortQueueLimitRespected();
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

 protected:
  static void sendPacket(
      facebook::fboss::AgentEnsemble* ensemble,
      const folly::IPAddressV6& dstIp) {
    folly::IPAddressV6 kSrcIp("2402::1");
    const auto dstMac = utility::getMacForFirstInterfaceWithPorts(
        ensemble->getProgrammedState());
    const auto srcMac = utility::MacAddressGenerator().get(dstMac.u64NBO() + 1);

    auto txPacket = utility::makeUDPTxPacket(
        ensemble->getSw(),
        std::nullopt, // vlanID
        srcMac,
        dstMac,
        kSrcIp,
        dstIp,
        8000, // l4 src port
        8001, // l4 dst port
        kLosslessPriority * 8 << 2, // dscp for lossless traffic
        255, // hopLimit
        std::vector<uint8_t>(4500));
    // Forward the packet in the pipeline
    ensemble->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
  }

  void verifySflowEgressPortNotStuck(int iterations) {
    if (checkSameAndGetAsic()->isSupported(
            HwAsic::Feature::EVENTOR_PORT_FOR_SFLOW)) {
      // Ensure eventor port stats keep incrementing
      const PortID kEventorPortId =
          masterLogicalPortIds({cfg::PortType::EVENTOR_PORT})[0];
      auto eventorStatsBefore = getLatestPortStats(kEventorPortId);
      WITH_RETRIES({
        auto eventorStatsAfter = getLatestPortStats(kEventorPortId);
        XLOG(DBG0) << "Eventor port outPackets: "
                   << *eventorStatsAfter.outUnicastPkts_();
        EXPECT_EVENTUALLY_GT(
            *eventorStatsAfter.outUnicastPkts_(),
            *eventorStatsBefore.outUnicastPkts_());
      });
    }
    auto portId = getNonSflowSampledInterfacePort();
    // Expect atleast 1Gbps of mirror traffic!
    const uint64_t kDesiredMirroredTrafficRate{1000000000};
    EXPECT_NO_THROW(getAgentEnsemble()->waitForSpecificRateOnPort(
        portId, kDesiredMirroredTrafficRate));
    // Make sure that we can sustain the rate for longer duration
    constexpr int kWaitPeriod{5};
    auto prevPortStats = getLatestPortStats(portId);
    for (int iter = 0; iter < iterations; iter++) {
      sleep(kWaitPeriod);
      auto curPortStats = getLatestPortStats(portId);
      auto rate = getAgentEnsemble()->getTrafficRate(
          prevPortStats, curPortStats, kWaitPeriod);
      // Ensure that we always see greater than the desired rate
      EXPECT_GT(rate, kDesiredMirroredTrafficRate);
      prevPortStats = curPortStats;
    }
  }

  void validateEventorPortQueueLimitRespected() {
    auto config{initialConfig(*getAgentEnsemble())};
    uint32_t maxExpectedQueueLimitBytes{0};
    PortID eventorPortId;
    for (auto& port : *config.ports()) {
      if (*port.portType() == cfg::PortType::EVENTOR_PORT) {
        auto voqConfigName = *port.portVoqConfigName();
        for (auto& voqConfig : config.portQueueConfigs()[voqConfigName]) {
          // Set the expected queue limit bytes to be 15% higher than
          // what we are configuring, as queue limit is not always
          // respected accurately.
          if (voqConfig.id() == 0) {
            maxExpectedQueueLimitBytes =
                *voqConfig.maxDynamicSharedBytes() * 1.15;
            break;
          }
        }
        eventorPortId = *port.logicalID();
        break;
      }
    }
    EXPECT_GT(maxExpectedQueueLimitBytes, 0);
    auto eventorSysPortId = getSystemPortID(
        eventorPortId,
        getProgrammedState(),
        SwitchID(*checkSameAndGetAsic()->getSwitchId()));
    WITH_RETRIES({
      auto latestSysPortStats = getLatestSysPortStats(eventorSysPortId);
      auto watermarkBytes = latestSysPortStats.queueWatermarkBytes_()->at(0);
      EXPECT_EVENTUALLY_GT(watermarkBytes, 0);
      EXPECT_LT(watermarkBytes, maxExpectedQueueLimitBytes);
    });
  }
};

class AgentSflowMirrorTruncateWithSamplesPackingTestV6
    : public AgentSflowMirrorTruncateTest<folly::IPAddressV6> {
 public:
  const int kNumSflowSamplesPacked{4};
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    auto productionFeatures = AgentSflowMirrorTruncateTest<
        folly::IPAddressV6>::getProductionFeaturesVerified();
    productionFeatures.push_back(ProductionFeature::SFLOW_SAMPLES_PACKING);
    return productionFeatures;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config =
        AgentSflowMirrorTruncateTest<folly::IPAddressV6>::initialConfig(
            ensemble);
    config.switchSettings()->numberOfSflowSamplesPerPacket() =
        kNumSflowSamplesPacked;
    return config;
  }

  void verifySflowSamplePacking(uint8_t numSflowSamplesPacked) {
    auto ports = getPortsForSampling();
    getAgentEnsemble()->bringDownPorts(
        std::vector<PortID>(ports.begin() + 2, ports.end()));
    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    // Send as many packets as the samples packed in a single
    // packet and expect to receive a single sampled packet.
    int numPacketsToSend{numSflowSamplesPacked};
    for (auto i = 0; i < numPacketsToSend; i++) {
      auto pkt = genPacket(1, 256);
      XLOG(DBG2) << "Sending packet through port " << ports[1];
      getAgentEnsemble()->sendPacketAsync(
          std::move(pkt), PortDescriptor(ports[1]), std::nullopt);
    }
    WITH_RETRIES({
      // Make sure that packets TXed from port stats!
      auto portStats = getLatestPortStats(ports[1]);
      EXPECT_EVENTUALLY_GE(*portStats.outUnicastPkts_(), numPacketsToSend);
    });
    int sflowPacketCount{0};
    std::optional<std::unique_ptr<folly::IOBuf>> capturedPacketBuf;
    // Should not expect to see more than number of packets sent
    for (int i = 0; i < numPacketsToSend; i++) {
      capturedPacketBuf = snooper.waitForPacket(1);
      if (capturedPacketBuf.has_value()) {
        sflowPacketCount++;
      }
    }
    // Make sure that we received exactly one sflow packet
    EXPECT_EQ(sflowPacketCount, 1);
    // Verify packing!
    // TODO: Inspect packet for the samples!
  }
};

using AgentSflowMirrorUntruncateTestV4 =
    AgentSflowMirrorUntruncateTest<folly::IPAddressV4>;
using AgentSflowMirrorUntruncateTestV6 =
    AgentSflowMirrorUntruncateTest<folly::IPAddressV6>;
using AgentSflowMirrorTruncateTestV4 =
    AgentSflowMirrorTruncateTest<folly::IPAddressV4>;
using AgentSflowMirrorTruncateTestV6 =
    AgentSflowMirrorTruncateTest<folly::IPAddressV6>;
using AgentSflowMirrorOnTrunkTestV4 =
    AgentSflowMirrorOnTrunkTest<folly::IPAddressV4>;
using AgentSflowMirrorOnTrunkTestV6 =
    AgentSflowMirrorOnTrunkTest<folly::IPAddressV6>;

#define SFLOW_SAMPLING_TEST(fixture, name, code) \
  TEST_F(fixture, name) {                        \
    { code }                                     \
  }

#define SFLOW_SAMPLING_UNTRUNCATE_TEST_V4_V6(name, code)            \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorUntruncateTestV4, name, code) \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorUntruncateTestV6, name, code)

#define SFLOW_SAMPLING_TRUNCATE_TEST_V4_V6(name, code)            \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorTruncateTestV4, name, code) \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorTruncateTestV6, name, code)

#define SFLOW_SAMPLING_TRUNK_TEST_V4_V6(name, code)              \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorOnTrunkTestV4, name, code) \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorOnTrunkTestV6, name, code)

SFLOW_SAMPLING_UNTRUNCATE_TEST_V4_V6(VerifySampledPacket, {
  this->testSampledPacket();
})
SFLOW_SAMPLING_UNTRUNCATE_TEST_V4_V6(VerifySampledPacketRate, {
  this->testSampledPacketRate();
})
SFLOW_SAMPLING_UNTRUNCATE_TEST_V4_V6(StressRecreateMirror, {
  this->stressRecreateMirror();
})

SFLOW_SAMPLING_TRUNCATE_TEST_V4_V6(VerifyTruncate, {
  this->testSampledPacket();
})
SFLOW_SAMPLING_TRUNCATE_TEST_V4_V6(VerifySampledPacketRate, {
  this->testSampledPacketRate();
})
SFLOW_SAMPLING_TRUNCATE_TEST_V4_V6(StressRecreateMirror, {
  this->stressRecreateMirror();
})

SFLOW_SAMPLING_TRUNK_TEST_V4_V6(VerifySampledPacket, {
  this->testSampledPacket();
})
SFLOW_SAMPLING_TRUNK_TEST_V4_V6(VerifySampledPacketRate, {
  this->testSampledPacketRate();
})

TEST_F(AgentSflowMirrorWithLineRateTrafficTest, VerifySflowEgressCongestion) {
  this->testSflowEgressCongestion(6);
}

TEST_F(
    AgentSflowMirrorWithLineRateTrafficTest,
    VerifySflowEgressCongestionShort) {
  // Test fewer iterations as we will be using this in an n-warmboot test.
  this->testSflowEgressCongestion(1);
}

class AgentSflowMirrorEventorTest
    : public AgentSflowMirrorWithLineRateTrafficTest {
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    auto productionFeatures = AgentSflowMirrorWithLineRateTrafficTest::
        getProductionFeaturesVerified();
    productionFeatures.push_back(
        ProductionFeature::SFLOW_SAMPLES_PACKING); // implies eventor port
    return productionFeatures;
  }

  void setCmdLineFlagOverrides() const override {
    AgentSflowMirrorWithLineRateTrafficTest::setCmdLineFlagOverrides();
    // Prevent neighbor advertisement traffic from generating stray packets.
    FLAGS_disable_neighbor_updates = true;
  }

 protected:
  void setSflowTruncation(bool truncate) {
    // Set truncation using shell command, since this is not supported by SDK.
    std::string out;
    for (int core = 0; core < 4; ++core) {
      for (const auto& [switchId, _] : getAsics()) {
        getAgentEnsemble()->runDiagCommand(
            fmt::format(
                "dbal entry commit table=SNIF_COMMAND_TABLE core_id={} "
                "snif_command_id=1 SNIF_TYPE=STATISTICAL_SAMPLE CROP_ENABLE={}\n",
                core,
                truncate ? "1" : "0"),
            out,
            switchId);
      }
    }
  }

  // Verify that packet count on a port remains constant for several iterations.
  void verifyNoTrafficOnPort(PortID portId, int iterations = 5) {
    auto statsBefore = getLatestPortStats(portId);
    int count = 0;
    WITH_RETRIES({
      auto statsAfter = getLatestPortStats(portId);
      XLOG(DBG0) << "Port " << portId
                 << " outPackets: " << *statsAfter.outUnicastPkts_();
      if (*statsBefore.outUnicastPkts_() == *statsAfter.outUnicastPkts_()) {
        ++count;
      } else {
        // Reset the count.
        statsBefore = statsAfter;
        count = 0;
      }
      EXPECT_EVENTUALLY_GE(count, iterations);
    });
  }

  // Stop traffic on ports by removing neighbor and adding it back.
  void stopTrafficOnAllPorts() {
    auto intfMac = utility::getMacForFirstInterfaceWithPorts(
        getAgentEnsemble()->getProgrammedState());
    utility::EcmpSetupTargetedPorts6 ecmpHelper(
        getAgentEnsemble()->getProgrammedState(),
        getAgentEnsemble()->getSw()->needL2EntryForNeighbor(),
        intfMac);
    for (auto portId : masterLogicalInterfacePortIds()) {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.unresolveNextHops(in, {PortDescriptor(portId)});
      });
    }
    verifyNoTrafficOnPort(masterLogicalInterfacePortIds()[0], 3 /*iterations*/);
    for (auto portId : masterLogicalInterfacePortIds()) {
      applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(in, {PortDescriptor(portId)});
      });
    }
  }
};

TEST_F(AgentSflowMirrorEventorTest, VerifyEventorMtu) {
  constexpr int kNumPorts = 1;
  const PortID trafficLoopPortId = masterLogicalInterfacePortIds()[0];
  const PortID eventorPortId =
      masterLogicalPortIds({cfg::PortType::EVENTOR_PORT})[0];

  auto setup = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    configureMirrorWithSampling(config, 1 /*sampleRate*/);
    utility::setTTLZeroCpuConfig(getAgentEnsemble()->getL3Asics(), config);
    applyNewConfig(config);
    resolveRouteForMirrorDestination();
    utility::setupEcmpDataplaneLoopOnAllPorts(getAgentEnsemble());
  };
  auto verify = [=, this]() {
    // Turn off truncation, then start traffic loops on a port. We don't expect
    // sFlow egress traffic because packets should be blocked by MTU.
    setSflowTruncation(false);
    utility::createTrafficOnMultiplePorts(
        getAgentEnsemble(), kNumPorts, sendPacket, 1 /*desiredPctLineRate*/);
    verifyNoTrafficOnPort(eventorPortId);

    // Stop traffic before changing truncation setting.
    stopTrafficOnAllPorts();
    verifyNoTrafficOnPort(trafficLoopPortId);

    // Turn truncation back on and start traffic again. We expect sFlow to work
    // now, i.e. eventor port should not be broken by previous large packets.
    setSflowTruncation(true);
    utility::createTrafficOnMultiplePorts(
        getAgentEnsemble(), kNumPorts, sendPacket, 1 /*desiredPctLineRate*/);
    verifySflowEgressPortNotStuck(3 /* iterations */);

    // Turn off traffic so that truncation can be changed after warmboot.
    stopTrafficOnAllPorts();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentSflowMirrorAddressFamilySwitchingTest, MoveToV6) {
  const bool kIsV4 = true;
  // Test to migrate v4 mirror to v6
  auto setup = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    configureMirrorWithSampling(config, 1 /* sampleRate */, kIsV4);
    configureTrapAcl(config, kIsV4);
    applyNewConfig(config);
    resolveRouteForMirrorDestination(kIsV4);
  };
  auto verify = [=, this]() { verifySampledPacketWithTruncate(kIsV4); };
  auto setupPostWb = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    config.mirrors()->clear();
    /* move to v6 */
    configureMirrorWithSampling(config, 1 /* sampleRate */, !kIsV4);
    configureTrapAcl(config, !kIsV4);
    applyNewConfig(config);
    resolveRouteForMirrorDestination(!kIsV4);
  };
  auto verifyPostWb = [=, this]() { verifySampledPacketWithTruncate(!kIsV4); };
  verifyAcrossWarmBoots(setup, verify, setupPostWb, verifyPostWb);
}

TEST_F(AgentSflowMirrorAddressFamilySwitchingTest, MoveToV4) {
  const bool kIsV4 = true;
  // Test to migrate v6 mirror to v4
  auto setup = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    configureMirrorWithSampling(config, 1 /* sampleRate */, !kIsV4);
    configureTrapAcl(config, !kIsV4);
    applyNewConfig(config);
    resolveRouteForMirrorDestination(!kIsV4);
  };
  auto verify = [=, this]() { verifySampledPacketWithTruncate(!kIsV4); };
  auto setupPostWb = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    config.mirrors()->clear();
    /* move to v4 */
    configureMirrorWithSampling(config, 1 /* sampleRate */, kIsV4);
    configureTrapAcl(config, kIsV4);
    applyNewConfig(config);
    resolveRouteForMirrorDestination(kIsV4);
  };
  auto verifyPostWb = [=, this]() { verifySampledPacketWithTruncate(kIsV4); };
  verifyAcrossWarmBoots(setup, verify, setupPostWb, verifyPostWb);
}

TEST_F(AgentSflowMirrorTruncateTestV6, verifyL4SrcPortRandomization) {
  auto setup = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    configureMirrorWithSampling(
        config, 1 /* sampleRate */, false /* v4 */, 0 /* udpSrcPort */);
    configureTrapAcl(config);
    applyNewConfig(config);
    resolveRouteForMirrorDestination();
  };
  auto verify = [=, this]() {
    this->verifySrcPortRandomizationOnSflowPacket();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(
    AgentSflowMirrorTruncateWithSamplesPackingTestV6,
    verifySflowSamplesPacking) {
  auto setup = [=, this]() {
    auto config = initialConfig(*getAgentEnsemble());
    configureMirror(config, false, 0);
    configSampling(config, 1);
    configureTrapAcl(config);
    applyNewConfig(config);
    resolveRouteForMirrorDestination();
  };
  auto verify = [=, this]() {
    this->verifySflowSamplePacking(kNumSflowSamplesPacked);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
