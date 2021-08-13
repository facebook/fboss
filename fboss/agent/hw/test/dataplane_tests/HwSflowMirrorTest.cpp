// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "folly/IPAddress.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using namespace ::testing;

DEFINE_int32(sflow_test_rate, 90000, "sflow sampling rate for hw test");
DEFINE_int32(sflow_test_time, 5, "sflow test traffic time in seconds");

namespace facebook::fboss {
class HwSflowMirrorTest : public HwLinkStateDependentTest {
 protected:
  utility::EthFrame genPacket(int portIndex, size_t payloadSize) {
    auto vlanId = utility::kBaseVlanId + portIndex;
    auto mac = utility::getInterfaceMac(
        getProgrammedState(), static_cast<VlanID>(vlanId));
    folly::IPAddressV6 sip{"2401:db00:dead:beef::2401"};
    folly::IPAddressV6 dip{folly::to<std::string>(kIpStr, portIndex, "::1")};
    uint16_t sport = 9701;
    uint16_t dport = 9801;
    return utility::getEthFrame(
        mac, mac, sip, dip, sport, dport, VlanID(vlanId), payloadSize);
  }

  std::vector<PortID> getPortsForSampling() const {
    auto portIds = masterLogicalPortIds();
    /*
     * Tajo does not share sample packet session yet and creates
     * one per port and the maximum is 16. Configure 16 ports
     * for now on Tajo to enable sflow with mirroring.
     */
    return getPlatform()->getAsic()->getAsicType() ==
            HwAsic::AsicType::ASIC_TYPE_TAJO
        ? std::vector<PortID>(portIds.begin(), portIds.begin() + 16)
        : portIds;
  }

  void sendPkt(PortID port, std::unique_ptr<TxPacket> pkt) {
    CHECK(port != getPortsForSampling()[0]);
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), port);
  }

  PortID getSlfowPacketSrcPort(const std::vector<uint8_t>& sflowPayload) {
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
    if (getPlatform()->getAsic()->getAsicType() ==
        HwAsic::AsicType::ASIC_TYPE_TAJO) {
      auto systemPortId = sflowPayload[0] << 8 | sflowPayload[1];
      return static_cast<PortID>(
          systemPortId - getPlatform()->getAsic()->getSystemPortIDOffset());
    } else {
      auto sourcePortOffset = 0;
      if (getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD)) {
        sourcePortOffset += 4;
      }
      return static_cast<PortID>(sflowPayload[sourcePortOffset]);
    }
  }

  int getSflowPacketHeaderLength(bool isV6 = false) {
    auto ipHeader = isV6 ? 40 : 20;
    int slfowShimHeaderLength =
        getPlatform()->getAsic()->getSflowShimHeaderSize();
    if (getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::SFLOW_SHIM_VERSION_FIELD)) {
      slfowShimHeaderLength += 4;
    }
    return 18 /* ethernet header */ + ipHeader + 8 /* udp header */ +
        slfowShimHeaderLength;
  }

  void generateTraffic(size_t payloadSize = 1400) {
    auto ports = getPortsForSampling();
    for (auto i = 1; i < ports.size(); i++) {
      auto pkt = genPacket(i, payloadSize);
      sendPkt(ports[i], pkt.getTxPacket(getHwSwitch()));
    }
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(), getPortsForSampling(), cfg::PortLoopbackMode::MAC);
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  void configTrunk(cfg::SwitchConfig* config) {
    utility::addAggPort(1, {getPortsForSampling()[0]}, config);
  }

  void
  configMirror(cfg::SwitchConfig* config, bool truncate, bool isV4 = true) {
    cfg::SflowTunnel sflowTunnel;
    sflowTunnel.ip_ref() = isV4 ? "101.101.101.101" : "2401:101:101::101";
    sflowTunnel.udpSrcPort_ref() = 6545;
    sflowTunnel.udpDstPort_ref() = 5343;

    cfg::MirrorTunnel tunnel;
    tunnel.sflowTunnel_ref() = sflowTunnel;

    cfg::MirrorDestination destination;
    destination.tunnel_ref() = tunnel;

    config->mirrors_ref()->resize(1);
    config->mirrors_ref()[0].name_ref() = "mirror";
    config->mirrors_ref()[0].destination_ref() = destination;
    config->mirrors_ref()[0].truncate_ref() = truncate;
  }

  void configSampling(cfg::SwitchConfig* config, int sampleRate) const {
    for (auto i = 1; i < getPortsForSampling().size(); i++) {
      auto portId = getPortsForSampling()[i];
      auto portCfg = utility::findCfgPort(*config, portId);
      portCfg->sFlowIngressRate_ref() = sampleRate;
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      portCfg->ingressMirror_ref() = "mirror";
    }
  }

  void resolveMirror() {
    auto vlanId = utility::firstVlanID(initialConfig());
    auto mac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto state = getProgrammedState()->clone();
    auto mirrors = state->getMirrors()->clone();
    auto mirror = mirrors->getMirrorIf("mirror")->clone();
    ASSERT_NE(mirror, nullptr);

    auto ip = mirror->getDestinationIp().value();
    if (ip.isV4()) {
      mirror->setMirrorTunnel(MirrorTunnel(
          folly::IPAddress("101.1.1.101"),
          mirror->getDestinationIp().value(),
          mac,
          mac,
          mirror->getTunnelUdpPorts().value()));
    } else {
      mirror->setMirrorTunnel(MirrorTunnel(
          folly::IPAddress("2401:101:1:1::101"),
          mirror->getDestinationIp().value(),
          mac,
          mac,
          mirror->getTunnelUdpPorts().value()));
    }

    mirror->setEgressPort(getPortsForSampling()[0]);
    mirrors->updateNode(mirror);
    state->resetMirrors(mirrors);
    applyNewState(state);
  }

  void setupRoutes(bool disableTTL = true) {
    auto ports = getPortsForSampling();
    auto size = ports.size();
    boost::container::flat_set<PortDescriptor> nhops;
    for (auto i = 1; i < size; i++) {
      nhops.insert(PortDescriptor(ports[i]));
    }

    auto vlanId = utility::firstVlanID(initialConfig());
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> helper6{
        getProgrammedState(),
        utility::getInterfaceMac(getProgrammedState(), vlanId)};
    auto state = helper6.resolveNextHops(getProgrammedState(), nhops);
    state = applyNewState(state);

    if (disableTTL) {
      for (const auto& nhop : helper6.getNextHops()) {
        if (nhop.portDesc == PortDescriptor(ports[0])) {
          continue;
        }
        utility::disableTTLDecrements(
            getHwSwitch(), helper6.getRouterId(), nhop);
      }
    }

    helper6.programRoutes(getRouteUpdater(), nhops);
    state = getProgrammedState();
    for (auto i = 1; i < size; i++) {
      boost::container::flat_set<PortDescriptor> port;
      port.insert(PortDescriptor(ports[i]));
      helper6.programRoutes(
          getRouteUpdater(),
          {PortDescriptor(ports[i])},
          {RouteV6::Prefix{
              folly::IPAddressV6(folly::to<std::string>(kIpStr, i, "::0")),
              80}});
      state = getProgrammedState();
    }
  }

  uint64_t getSampleCount(const std::map<PortID, HwPortStats>& stats) {
    auto portStats = stats.at(getPortsForSampling()[0]);
    return *portStats.outUnicastPkts__ref();
  }

  uint64_t getExpectedSampleCount(const std::map<PortID, HwPortStats>& stats) {
    uint64_t expectedSampleCount = 0;
    uint64_t allPortRx = 0;
    for (auto i = 1; i < getPortsForSampling().size(); i++) {
      auto port = getPortsForSampling()[i];
      auto portStats = stats.at(port);
      allPortRx += *portStats.inUnicastPkts__ref();
      expectedSampleCount +=
          (*portStats.inUnicastPkts__ref() / FLAGS_sflow_test_rate);
    }
    XLOG(INFO) << "total packets rx " << allPortRx;
    return expectedSampleCount;
  }

  void runSampleRateTest(size_t payloadSize = kDefaultPayloadSize) {
    if (!getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::SFLOW_SAMPLING)) {
      return;
    }
    if (payloadSize > kDefaultPayloadSize &&
        !getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::MIRROR_PACKET_TRUNCATION)) {
      return;
    }
    auto setup = [=]() {
      auto config = initialConfig();
      configMirror(&config, false);
      configSampling(&config, FLAGS_sflow_test_rate);
      applyNewConfig(config);
      setupRoutes();
      resolveMirror();
    };
    auto verify = [=]() {
      generateTraffic(payloadSize);
      std::this_thread::sleep_for(std::chrono::seconds(FLAGS_sflow_test_time));
      auto ports = getPortsForSampling();
      bringDownPorts(std::vector<PortID>(ports.begin() + 1, ports.end()));
      auto stats = getLatestPortStats(getPortsForSampling());
      auto actualSampleCount = getSampleCount(stats);
      auto expectedSampleCount = getExpectedSampleCount(stats);
      EXPECT_NE(actualSampleCount, 0);
      EXPECT_NE(expectedSampleCount, 0);
      auto difference = (expectedSampleCount > actualSampleCount)
          ? (expectedSampleCount - actualSampleCount)
          : (actualSampleCount - expectedSampleCount);
      auto percentError = (difference * 100) / actualSampleCount;
      EXPECT_LE(percentError, 5);
      XLOG(INFO) << "expected number of " << expectedSampleCount << " samples";
      XLOG(INFO) << "captured number of " << actualSampleCount << " samples";
    };
    verifyAcrossWarmBoots(setup, verify);
  }

  constexpr static size_t kDefaultPayloadSize = 1400;
  constexpr static auto kIpStr = "2401:db00:dead:beef:";
};

TEST_F(HwSflowMirrorTest, VerifySampledPacket) {
  if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::SFLOW_SAMPLING)) {
    return;
  }
  auto setup = [=]() {
    auto config = initialConfig();
    configMirror(&config, false);
    configSampling(&config, 1);
    applyNewConfig(config);
    resolveMirror();
  };
  auto verify = [=]() {
    auto ports = getPortsForSampling();
    bringDownPorts(std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 256);
    auto dstIp = folly::CIDRNetwork{"101.101.101.101", 32};
    auto packetCapture = HwTestPacketTrapEntry(getHwSwitch(), dstIp);
    HwTestPacketSnooper snooper(getHwSwitchEnsemble());
    sendPkt(getPortsForSampling()[1], pkt.getTxPacket(getHwSwitch()));
    auto capturedPkt = snooper.waitForPacket(10);
    ASSERT_TRUE(capturedPkt.has_value());

    // captured packet has encap header on top
    ASSERT_GE(capturedPkt->length(), pkt.length());
    EXPECT_GE(capturedPkt->length(), getSflowPacketHeaderLength());

    auto delta = capturedPkt->length() - pkt.length();
    EXPECT_EQ(
        delta,
        getSflowPacketHeaderLength() -
            4 /* vlan tag is absent in mirrored packet */);
    auto payload = capturedPkt->v4PayLoad()->payload()->payload();

    EXPECT_EQ(getSlfowPacketSrcPort(payload), getPortsForSampling()[1]);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwSflowMirrorTest, VerifySampledPacketWithTruncateV4) {
  if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::SFLOW_SAMPLING) ||
      !getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::MIRROR_PACKET_TRUNCATION)) {
    return;
  }
  auto setup = [=]() {
    auto config = initialConfig();
    configMirror(&config, true);
    configSampling(&config, 1);
    applyNewConfig(config);
    resolveMirror();
  };
  auto verify = [=]() {
    auto ports = getPortsForSampling();
    bringDownPorts(std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 8000);
    auto dstIp = folly::CIDRNetwork{"101.101.101.101", 32};
    auto packetCapture = HwTestPacketTrapEntry(getHwSwitch(), dstIp);
    HwTestPacketSnooper snooper(getHwSwitchEnsemble());
    sendPkt(getPortsForSampling()[1], pkt.getTxPacket(getHwSwitch()));
    auto capturedPkt = snooper.waitForPacket(10);
    ASSERT_TRUE(capturedPkt.has_value());

    auto _ = capturedPkt->getTxPacket(getHwSwitch());
    auto __ = folly::io::Cursor(_->buf());
    XLOG(INFO) << PktUtil::hexDump(__);

    // packet's payload is truncated before it was mirrored
    EXPECT_LE(capturedPkt->length(), pkt.length());
    auto capturedHdrSize = getSflowPacketHeaderLength();
    EXPECT_GE(capturedPkt->length(), capturedHdrSize);
    EXPECT_LE(
        capturedPkt->length() - capturedHdrSize,
        216); /* TODO: confirm length in CS00010399535 and CS00012130950  */
    auto payload = capturedPkt->v4PayLoad()->payload()->payload();
    EXPECT_EQ(getSlfowPacketSrcPort(payload), getPortsForSampling()[1]);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwSflowMirrorTest, VerifySampledPacketWithTruncateV6) {
  if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::SFLOW_SAMPLING) ||
      !getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::MIRROR_PACKET_TRUNCATION) ||
      !getPlatform()->getAsic()->isSupported(HwAsic::Feature::SFLOWv6)) {
    return;
  }
  auto setup = [=]() {
    auto config = initialConfig();
    configMirror(&config, true, false);
    configSampling(&config, 1);
    applyNewConfig(config);
    resolveMirror();
  };
  auto verify = [=]() {
    auto ports = getPortsForSampling();
    bringDownPorts(std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 8000);
    auto dstIp = folly::CIDRNetwork{"2401:101:101::101", 128};
    auto packetCapture = HwTestPacketTrapEntry(getHwSwitch(), dstIp);
    HwTestPacketSnooper snooper(getHwSwitchEnsemble());
    sendPkt(getPortsForSampling()[1], pkt.getTxPacket(getHwSwitch()));
    auto capturedPkt = snooper.waitForPacket(10);
    ASSERT_TRUE(capturedPkt.has_value());

    auto _ = capturedPkt->getTxPacket(getHwSwitch());
    auto __ = folly::io::Cursor(_->buf());
    XLOG(INFO) << PktUtil::hexDump(__);

    // packet's payload is truncated before it was mirrored
    EXPECT_LE(capturedPkt->length(), pkt.length());
    auto capturedHdrSize = getSflowPacketHeaderLength(true);
    EXPECT_GE(capturedPkt->length(), capturedHdrSize);
    EXPECT_LE(
        capturedPkt->length() - capturedHdrSize,
        216); /* TODO: confirm length in CS00010399535 and CS00012130950 */
    auto payload = capturedPkt->v6PayLoad()->payload()->payload();
    EXPECT_EQ(getSlfowPacketSrcPort(payload), getPortsForSampling()[1]);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwSflowMirrorTest, VerifySampledPacketCount) {
  runSampleRateTest();
}

TEST_F(HwSflowMirrorTest, VerifySampledPacketCountWithLargePackets) {
  runSampleRateTest(8192);
}

TEST_F(HwSflowMirrorTest, VerifySampledPacketWithLagMemberAsEgressPort) {
  if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::SFLOW_SAMPLING) ||
      !getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::MIRROR_PACKET_TRUNCATION) ||
      !getPlatform()->getAsic()->isSupported(HwAsic::Feature::SFLOWv6)) {
    return;
  }
  auto setup = [=]() {
    auto config = initialConfig();
    configTrunk(&config);
    configMirror(&config, true /* truncate */, false /* isv6 */);
    configSampling(&config, 1);
    auto state = applyNewConfig(config);
    applyNewState(utility::enableTrunkPorts(state));
    resolveMirror();
  };
  auto verify = [=]() {
    auto ports = getPortsForSampling();
    bringDownPorts(std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 8000);
    auto dstIp = folly::CIDRNetwork{"2401:101:101::101", 128};
    auto packetCapture = HwTestPacketTrapEntry(getHwSwitch(), dstIp);
    HwTestPacketSnooper snooper(getHwSwitchEnsemble());
    sendPkt(getPortsForSampling()[1], pkt.getTxPacket(getHwSwitch()));
    auto capturedPkt = snooper.waitForPacket(10);
    ASSERT_TRUE(capturedPkt.has_value());

    auto _ = capturedPkt->getTxPacket(getHwSwitch());
    auto __ = folly::io::Cursor(_->buf());
    XLOG(INFO) << PktUtil::hexDump(__);

    // packet's payload is truncated before it was mirrored
    EXPECT_LE(capturedPkt->length(), pkt.length());
    auto capturedHdrSize = getSflowPacketHeaderLength(true);
    EXPECT_GE(capturedPkt->length(), capturedHdrSize);
    EXPECT_LE(
        capturedPkt->length() - capturedHdrSize,
        216); /* TODO: confirm length in CS00010399535 and CS00012130950 */
    auto payload = capturedPkt->v6PayLoad()->payload()->payload();
    EXPECT_EQ(getSlfowPacketSrcPort(payload), getPortsForSampling()[1]);
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
