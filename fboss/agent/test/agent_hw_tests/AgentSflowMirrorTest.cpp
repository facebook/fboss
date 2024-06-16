// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/IPAddressV4.h>
#include <type_traits>
#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"

#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/SflowShimUtils.h"

DEFINE_int32(sflow_test_rate, 90000, "sflow sampling rate for hw test");

const std::string kSflowMirror = "sflow_mirror";

namespace facebook::fboss {

template <typename AddrT>
class AgentSflowMirrorTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {production_features::ProductionFeature::SFLOWv4_SAMPLING};
    } else {
      return {production_features::ProductionFeature::SFLOWv6_SAMPLING};
    }
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);

    auto port0 = ensemble.masterLogicalPortIds()[0];
    auto port0Switch =
        ensemble.getSw()->getScopeResolver()->scope(port0).switchId();
    auto asic = ensemble.getSw()->getHwAsicTable()->getHwAsic(port0Switch);
    auto ports = getPortsForSampling(ensemble.masterLogicalPortIds(), asic);
    this->configureMirror(cfg);
    this->configureTrapAcl(cfg);
    configSampling(cfg, ports, 1);
    return cfg;
  }

  virtual void configureMirror(cfg::SwitchConfig& cfg) const {
    utility::configureSflowMirror(
        cfg, kSflowMirror, false, std::is_same_v<AddrT, folly::IPAddressV4>);
  }

  void configureTrapAcl(cfg::SwitchConfig& cfg) const {
    utility::configureTrapAcl(cfg, std::is_same_v<AddrT, folly::IPAddressV4>);
  }

  PortID getNonSflowSampledInterfacePorts() {
    return getPortsForSampling()[0];
  }

  std::vector<PortID> getPortsForSampling() const {
    auto portIds = masterLogicalPortIds();
    auto switchID = switchIdForPort(portIds[0]);
    auto asic = getSw()->getHwAsicTable()->getHwAsic(switchID);
    return getPortsForSampling(portIds, asic);
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

  void configSampling(
      cfg::SwitchConfig& config,
      const std::vector<PortID>& ports,
      int sampleRate) const {
    std::vector<PortID> samplePorts(ports.begin() + 1, ports.end());
    utility::configureSflowSampling(
        config, kSflowMirror, samplePorts, sampleRate);
  }

  void configSampling(cfg::SwitchConfig& config, int sampleRate) {
    auto ports = getPortsForSampling();
    configSampling(config, ports, sampleRate);
  }

  const HwAsic* getAsic() const {
    return getAgentEnsemble()->getSw()->getHwAsicTable()->getHwAsic(
        switchIdForPort(masterLogicalPortIds()[0]));
  }

  std::optional<uint32_t> getHwLogicalPortId(PortID port) const {
    return getSw()->getHwLogicalPortId(port);
  }

  PortID getSflowPacketSrcPort(const std::vector<uint8_t>& sflowPayload) {
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
    auto asic = getAsic();
    if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO) {
      auto systemPortId = sflowPayload[0] << 8 | sflowPayload[1];
      return static_cast<PortID>(systemPortId - asic->getSystemPortIDOffset());
    } else {
      auto sourcePortOffset = 0;
      if (asic->isSupported(HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD)) {
        sourcePortOffset += 4;
      }
      return static_cast<PortID>(sflowPayload[sourcePortOffset]);
    }
  }

  uint16_t getMirrorTruncateSize() const {
    return getAsic()->getMirrorTruncateSize();
  }

  std::shared_ptr<SwitchState> updateMacAddress(
      const std::shared_ptr<SwitchState>& state,
      AddrT ip,
      PortID egressPort,
      const std::optional<folly::MacAddress>& randomMac) {
    /* add tunnel destination ip as neighbor */
    auto outputState{state->clone()};

    using NeighborTableT = typename std::conditional_t<
        std::is_same<AddrT, folly::IPAddressV4>::value,
        ArpTable,
        NdpTable>;

    auto mac = randomMac.has_value() ? randomMac.value()
                                     : utility::getFirstInterfaceMac(state);
    auto port = outputState->getPorts()->getNodeIf(egressPort);
    auto vlanID = port->getIngressVlan();
    auto vlan = outputState->getVlans()->getNodeIf(vlanID);
    auto intf = outputState->getInterfaces()->getNodeIf(vlan->getInterfaceID());

    NeighborTableT* nbrTable;
    if (FLAGS_intf_nbr_tables) {
      nbrTable = intf->template getNeighborEntryTable<AddrT>()->modify(
          intf->getID(), &outputState);
    } else {
      nbrTable = vlan->template getNeighborEntryTable<AddrT>()->modify(
          vlan->getID(), &outputState);
    }

    auto entry = nbrTable->getEntryIf(ip);
    CHECK(entry);
    entry = entry->clone();
    entry->setMAC(mac);
    nbrTable->updateEntry(ip, entry);
    return outputState;
  }

  void resolveRouteForMirrorDestination() {
    const auto mirrorDestinationPort = getNonSflowSampledInterfacePorts();
    boost::container::flat_set<PortDescriptor> nhopPorts{
        PortDescriptor(mirrorDestinationPort)};

    this->getAgentEnsemble()->applyNewState(
        [&](const std::shared_ptr<SwitchState>& state) {
          utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(state);
          auto newState = ecmpHelper.resolveNextHops(state, nhopPorts);
          return newState;
        },
        "resolve mirror nexthop");

    auto mirror = getSw()->getState()->getMirrors()->getNodeIf(kSflowMirror);
    auto dip = mirror->getDestinationIp();

    RoutePrefix<AddrT> prefix(AddrT(dip->str()), dip->bitCount());
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(getProgrammedState());

    ecmpHelper.programRoutes(
        getAgentEnsemble()->getRouteUpdaterWrapper(), nhopPorts, {prefix});
  }

  utility::EthFrame genPacket(int portIndex, size_t payloadSize) {
    auto portId = masterLogicalPortIds()[portIndex];
    auto vlanId =
        getProgrammedState()->getPorts()->getNodeIf(portId)->getIngressVlan();
    auto mac = utility::getInterfaceMac(
        getProgrammedState(), static_cast<VlanID>(vlanId));
    folly::IPAddressV6 sip{"2401:db00:dead:beef::2401"};
    folly::IPAddressV6 dip{folly::to<std::string>(kIpStr, portIndex, "::1")};
    uint16_t sport = 9701;
    uint16_t dport = 9801;
    return utility::getEthFrame(
        mac, mac, sip, dip, sport, dport, VlanID(vlanId), payloadSize);
  }

  int getSflowPacketHeaderLength() {
    return getSflowPacketHeaderLength(
        std::is_same_v<AddrT, folly::IPAddressV6>);
  }

  int getSflowPacketHeaderLength(bool isV6) {
    auto ipHeader = isV6 ? 40 : 20;
    auto asic = getAsic();
    int slfowShimHeaderLength = asic->getSflowShimHeaderSize();
    if (asic->isSupported(HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD)) {
      slfowShimHeaderLength += 4;
    }
    return 18 /* ethernet header */ + ipHeader + 8 /* udp header */ +
        slfowShimHeaderLength;
  }

  void verifySampledPacket() {
    auto ports = getPortsForSampling();
    getAgentEnsemble()->bringDownPorts(
        std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 256);

    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    XLOG(DBG2) << "Sending packet through port " << ports[1];
    getAgentEnsemble()->sendPacketAsync(
        pkt.getTxPacket(
            [sw = getSw()](uint32_t size) { return sw->allocatePacket(size); }),
        PortDescriptor(ports[1]),
        std::nullopt);

    std::optional<std::unique_ptr<folly::IOBuf>> capturedPktBuf;
    WITH_RETRIES({
      capturedPktBuf = snooper.waitForPacket(10);
      EXPECT_EVENTUALLY_TRUE(capturedPktBuf.has_value());
    });
    folly::io::Cursor capturedPktCursor{capturedPktBuf->get()};
    auto capturedPkt = utility::EthFrame(capturedPktCursor);

    // captured packet has encap header on top
    ASSERT_GE(capturedPkt.length(), pkt.length());
    EXPECT_GE(capturedPkt.length(), getSflowPacketHeaderLength());

    auto delta = capturedPkt.length() - pkt.length();
    EXPECT_LE(delta, getSflowPacketHeaderLength());
    auto payload = std::is_same_v<AddrT, folly::IPAddressV4>
        ? capturedPkt.v4PayLoad()->udpPayload()->payload()
        : capturedPkt.v6PayLoad()->udpPayload()->payload();

    auto hwLogicalPortId = getHwLogicalPortId(ports[1]);
    if (!hwLogicalPortId) {
      EXPECT_EQ(getSflowPacketSrcPort(payload), ports[1]);

    } else {
      EXPECT_EQ(getSflowPacketSrcPort(payload), PortID(*hwLogicalPortId));
    }

    if (getAsic()->getAsicType() != cfg::AsicType::ASIC_TYPE_EBRO) {
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
      if (getAsic()->isSupported(HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD)) {
        EXPECT_EQ(shim.asic, utility::SflowShimAsic::SFLOW_SHIM_ASIC_TH4);
      } else {
        EXPECT_EQ(shim.asic, utility::SflowShimAsic::SFLOW_SHIM_ASIC_TH3);
      }
    }
  }

  void verifySampledPacketWithTruncate() {
    auto ports = getPortsForSampling();
    getAgentEnsemble()->bringDownPorts(
        std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 8000);

    utility::SwSwitchPacketSnooper snooper(getSw(), "snooper");
    XLOG(DBG2) << "Sending packet through port " << ports[1];
    getAgentEnsemble()->sendPacketAsync(
        pkt.getTxPacket(
            [sw = getSw()](uint32_t size) { return sw->allocatePacket(size); }),
        PortDescriptor(ports[1]),
        std::nullopt);

    std::optional<std::unique_ptr<folly::IOBuf>> capturedPktBuf;
    WITH_RETRIES({
      capturedPktBuf = snooper.waitForPacket(10);
      EXPECT_EVENTUALLY_TRUE(capturedPktBuf.has_value());
    });
    folly::io::Cursor capturedPktCursor{capturedPktBuf->get()};
    auto capturedPkt = utility::EthFrame(capturedPktCursor);

    // captured packet has encap header on top
    EXPECT_LE(capturedPkt.length(), pkt.length());
    auto capturedHdrSize = getSflowPacketHeaderLength(true);
    EXPECT_GE(capturedPkt.length(), capturedHdrSize);

    EXPECT_LE(
        capturedPkt.length() - capturedHdrSize,
        getMirrorTruncateSize()); /* TODO: confirm length in CS00010399535 and
                                     CS00012130950 */
  }

  uint64_t getSampleCount(const std::map<PortID, HwPortStats>& stats) {
    auto portStats = stats.at(getPortsForSampling()[0]);
    return *portStats.outUnicastPkts_();
  }

  uint64_t getExpectedSampleCount(const std::map<PortID, HwPortStats>& stats) {
    uint64_t expectedSampleCount = 0;
    uint64_t allPortRx = 0;
    for (auto i = 1; i < getPortsForSampling().size(); i++) {
      auto port = getPortsForSampling()[i];
      auto portStats = stats.at(port);
      allPortRx += *portStats.inUnicastPkts_();
      expectedSampleCount +=
          (*portStats.inUnicastPkts_() / FLAGS_sflow_test_rate);
    }
    XLOG(DBG2) << "total packets rx " << allPortRx;
    return expectedSampleCount;
  }

  void verifySampledPacketRate() {
    auto ports = getPortsForSampling();

    auto pkt = genPacket(1, 256);

    for (auto i = 1; i < ports.size(); i++) {
      getAgentEnsemble()->sendPacketAsync(
          pkt.getTxPacket([sw = getSw()](uint32_t size) {
            return sw->allocatePacket(size);
          }),
          PortDescriptor(ports[i]),
          std::nullopt);
      getAgentEnsemble()->waitForLineRateOnPort(ports[i]);
    }

    getAgentEnsemble()->bringDownPorts(
        std::vector<PortID>(ports.begin() + 1, ports.end()));

    WITH_RETRIES({
      auto stats = getLatestPortStats(getPortsForSampling());
      auto actualSampleCount = getSampleCount(stats);
      auto expectedSampleCount = getExpectedSampleCount(stats);
      auto difference = (expectedSampleCount > actualSampleCount)
          ? (expectedSampleCount - actualSampleCount)
          : (actualSampleCount - expectedSampleCount);
      if (!actualSampleCount) {
        continue;
      }
      auto percentError = (difference * 100) / actualSampleCount;
      EXPECT_EVENTUALLY_LE(percentError, kDefaultPercentErrorThreshold);
      XLOG(DBG2) << "expected number of " << expectedSampleCount << " samples";
      XLOG(DBG2) << "captured number of " << actualSampleCount << " samples";
    });

    getAgentEnsemble()->bringUpPorts(
        std::vector<PortID>(ports.begin() + 1, ports.end()));
    getAgentEnsemble()->clearPortStats();
  }

  virtual void testSampledPacket(bool truncate = false) {
    auto setup = [=, this]() {
      auto ports = getPortsForSampling();
      auto config = initialConfig(*getAgentEnsemble());
      configSampling(config, 1);
      applyNewConfig(config);
      resolveRouteForMirrorDestination();
    };
    auto verify = [=, this]() {
      if (!truncate) {
        verifySampledPacket();
      } else {
        verifySampledPacketWithTruncate();
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  void testSampledPacketRate(bool truncate = false) {
    auto setup = [=, this]() {
      auto config = initialConfig(*getAgentEnsemble());
      configSampling(config, FLAGS_sflow_test_rate);
      applyNewConfig(config);
      resolveRouteForMirrorDestination();
    };
    auto verify = [=, this]() { verifySampledPacketRate(); };
    verifyAcrossWarmBoots(setup, verify);
  }

  utility::MacAddressGenerator macGenerator = utility::MacAddressGenerator();
  constexpr static size_t kDefaultPayloadSize = 1400;
  constexpr static size_t kDefaultPercentErrorThreshold = 7;
  constexpr static auto kIpStr = "2401:db00:dead:beef:";
};

template <typename AddrT>
class AgentSflowMirrorTruncateTest : public AgentSflowMirrorTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {
          production_features::ProductionFeature::SFLOWv4_SAMPLING,
          production_features::ProductionFeature::MIRROR_PACKET_TRUNCATION};
    } else {
      return {
          production_features::ProductionFeature::SFLOWv6_SAMPLING,
          production_features::ProductionFeature::MIRROR_PACKET_TRUNCATION};
    }
  }

  virtual void configureMirror(cfg::SwitchConfig& cfg) const override {
    utility::configureSflowMirror(
        cfg,
        kSflowMirror,
        true /* truncate */,
        std::is_same_v<AddrT, folly::IPAddressV4>);
  }
};

template <typename AddrT>
class AgentSflowMirrorOnTrunkTest : public AgentSflowMirrorTruncateTest<AddrT> {
 public:
  using AgentSflowMirrorTest<AddrT>::getPortsForSampling;

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {
          production_features::ProductionFeature::SFLOWv4_SAMPLING,
          production_features::ProductionFeature::LAG,
          production_features::ProductionFeature::MIRROR_PACKET_TRUNCATION};
    } else {
      return {
          production_features::ProductionFeature::SFLOWv6_SAMPLING,
          production_features::ProductionFeature::LAG,
          production_features::ProductionFeature::MIRROR_PACKET_TRUNCATION};
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

using AgentSflowMirrorTestV4 = AgentSflowMirrorTest<folly::IPAddressV4>;
using AgentSflowMirrorTestV6 = AgentSflowMirrorTest<folly::IPAddressV6>;
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

#define SFLOW_SAMPLING_TEST_V4_V6(name, code)             \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorTestV4, name, code) \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorTestV6, name, code)

#define SFLOW_SAMPLING_TRUNCATE_TEST_V4_V6(name, code)            \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorTruncateTestV4, name, code) \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorTruncateTestV6, name, code)

#define SFLOW_SAMPLING_TRUNK_TEST_V4_V6(name, code)              \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorOnTrunkTestV4, name, code) \
  SFLOW_SAMPLING_TEST(AgentSflowMirrorOnTrunkTestV6, name, code)

SFLOW_SAMPLING_TEST_V4_V6(VerifySampledPacket, { this->testSampledPacket(); })
SFLOW_SAMPLING_TEST_V4_V6(VerifySampledPacketRate, {
  this->testSampledPacketRate();
})

SFLOW_SAMPLING_TRUNCATE_TEST_V4_V6(VerifyTruncate, {
  this->testSampledPacket(true);
})
SFLOW_SAMPLING_TRUNCATE_TEST_V4_V6(VerifySampledPacketRate, {
  this->testSampledPacketRate(true);
})

SFLOW_SAMPLING_TRUNK_TEST_V4_V6(VerifySampledPacket, {
  this->testSampledPacket(true);
})
SFLOW_SAMPLING_TRUNK_TEST_V4_V6(VerifySampledPacketRate, {
  this->testSampledPacketRate(true);
})

} // namespace facebook::fboss
