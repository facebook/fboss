// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"

#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmQosUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using namespace ::testing;

namespace {
constexpr auto kIpStr = "2401:db00:dead:beef:";
}

namespace facebook::fboss {
class BcmSflowMirrorTest : public BcmLinkStateDependentTests {
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
        mac, mac, sip, dip, sport, dport, vlanId, payloadSize);
  }

  void sendPkt(PortID port, std::unique_ptr<TxPacket> pkt) {
    CHECK(port != masterLogicalPortIds()[0]);
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), port);
  }

  void generateTraffic(size_t payloadSize = 1400) {
    auto ports = masterLogicalPortIds();
    for (auto i = 1; i < ports.size(); i++) {
      auto port = ports[i];
      auto pkt = genPacket(i, payloadSize);
      sendPkt(ports[i], pkt.getTxPacket(getHwSwitch()));
    }
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
  }

  uint32_t featuresDesired() const override {
    return (HwSwitch::LINKSCAN_DESIRED | HwSwitch::PACKET_RX_DESIRED);
  }

  void
  configMirror(cfg::SwitchConfig* config, bool truncate, bool isV4 = true) {
    cfg::SflowTunnel sflowTunnel;
    sflowTunnel.ip = isV4 ? "101.101.101.101" : "2401:101:101::101";
    sflowTunnel.udpSrcPort_ref() = 6545;
    sflowTunnel.udpDstPort_ref() = 5343;

    cfg::MirrorTunnel tunnel;
    tunnel.sflowTunnel_ref() = sflowTunnel;

    cfg::MirrorDestination destination;
    destination.tunnel_ref() = tunnel;

    config->mirrors.resize(1);
    config->mirrors[0].name = "mirror";
    config->mirrors[0].destination = destination;
    config->mirrors[0].truncate = truncate;
  }

  void configSampling(cfg::SwitchConfig* config, int sampleRate) const {
    for (auto i = 1; i < config->ports.size(); i++) {
      config->ports[i].sFlowIngressRate = sampleRate;
      config->ports[i].sampleDest_ref() = cfg::SampleDestination::MIRROR;
      config->ports[i].ingressMirror_ref() = "mirror";
    }
  }

  void configIngressMirrorOnPort(cfg::SwitchConfig* config, PortID port) const {
    for (auto i = 1; i < config->ports.size(); i++) {
      if (static_cast<PortID>(config->ports[i].logicalID) == port) {
        config->ports[i].ingressMirror_ref() = "mirror";
        config->ports[i].sampleDest_ref() = cfg::SampleDestination::MIRROR;
        config->ports[i].sFlowIngressRate = 1;
        break;
      }
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

    mirror->setEgressPort(masterLogicalPortIds()[0]);
    mirrors->updateNode(mirror);
    state->resetMirrors(mirrors);
    applyNewState(state);
  }
};

TEST_F(BcmSflowMirrorTest, VerifySampledPacket) {
  if (!getPlatform()->sflowSamplingSupported()) {
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
    auto ports = masterLogicalPortIds();
    bringDownPorts(std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 256);
    auto packetCapture =
        HwTestPacketTrapEntry(getHwSwitch(), masterLogicalPortIds()[0]);
    HwTestPacketSnooper snooper(getHwSwitchEnsemble());
    sendPkt(masterLogicalPortIds()[1], pkt.getTxPacket(getHwSwitch()));
    auto capturedPkt = snooper.waitForPacket(10);
    ASSERT_TRUE(capturedPkt.has_value());

    // captured packet has encap header on top
    ASSERT_GE(capturedPkt->length(), pkt.length());
    EXPECT_GE(
        capturedPkt->length(),
        18 /* ethernet */ + 20 /* v4 */ + 8 /* udp */ + 8 /* shim */);

    auto delta = capturedPkt->length() - pkt.length();
    EXPECT_EQ(
        delta,
        18 /* ethernet header */ + 20 /* ipv4 header */ + 8 /* udp header */ +
            8 /* sflow header */ -
            4 /* vlan tag is absent in mirrored packet */);
    auto payload = capturedPkt->v4PayLoad()->payload()->payload();

    // sflow shim format:
    //    sport : 8, smod : 8, dport : 8, dmod : 8
    //    source_sample : 1, dest_sample : 1, flex_sample : 1
    //    multicast : 1, discarded : 1, truncated : 1,
    //    dest_port_encoding : 3, reserved : 23
    EXPECT_EQ(static_cast<PortID>(payload[0]), masterLogicalPortIds()[1]);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmSflowMirrorTest, VerifySampledPacketWithTruncateV4) {
  if (!getPlatform()->sflowSamplingSupported() ||
      !getPlatform()->mirrorPktTruncationSupported()) {
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
    auto ports = masterLogicalPortIds();
    bringDownPorts(std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 8000);
    auto packetCapture =
        HwTestPacketTrapEntry(getHwSwitch(), masterLogicalPortIds()[0]);
    HwTestPacketSnooper snooper(getHwSwitchEnsemble());
    sendPkt(masterLogicalPortIds()[1], pkt.getTxPacket(getHwSwitch()));
    auto capturedPkt = snooper.waitForPacket(10);
    ASSERT_TRUE(capturedPkt.has_value());

    auto _ = capturedPkt->getTxPacket(getHwSwitch());
    auto __ = folly::io::Cursor(_->buf());
    XLOG(INFO) << PktUtil::hexDump(__);

    // packet's payload is truncated before it was mirrored
    EXPECT_LE(capturedPkt->length(), pkt.length());
    auto capturedHdrSize =
        18 /* ethernet */ + 20 /* ipv4 */ + 8 /* udp */ + 8 /* sflow */;
    EXPECT_GE(capturedPkt->length(), capturedHdrSize);
    EXPECT_EQ(capturedPkt->length() - capturedHdrSize, 210); /* TODO: why? */
    auto payload = capturedPkt->v4PayLoad()->payload()->payload();
    EXPECT_EQ(static_cast<PortID>(payload[0]), masterLogicalPortIds()[1]);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmSflowMirrorTest, VerifySampledPacketWithTruncateV6) {
  if (!getPlatform()->sflowSamplingSupported() ||
      !getPlatform()->mirrorPktTruncationSupported() ||
      !getPlatform()->v6MirrorTunnelSupported()) {
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
    auto ports = masterLogicalPortIds();
    bringDownPorts(std::vector<PortID>(ports.begin() + 2, ports.end()));
    auto pkt = genPacket(1, 8000);
    auto packetCapture =
        HwTestPacketTrapEntry(getHwSwitch(), masterLogicalPortIds()[0]);
    HwTestPacketSnooper snooper(getHwSwitchEnsemble());
    sendPkt(masterLogicalPortIds()[1], pkt.getTxPacket(getHwSwitch()));
    auto capturedPkt = snooper.waitForPacket(10);
    ASSERT_TRUE(capturedPkt.has_value());

    auto _ = capturedPkt->getTxPacket(getHwSwitch());
    auto __ = folly::io::Cursor(_->buf());
    XLOG(INFO) << PktUtil::hexDump(__);

    // packet's payload is truncated before it was mirrored
    EXPECT_LE(capturedPkt->length(), pkt.length());
    auto capturedHdrSize =
        18 /* ethernet */ + 40 /* ipv6 */ + 8 /* udp */ + 8 /* sflow */;
    EXPECT_GE(capturedPkt->length(), capturedHdrSize);
    EXPECT_EQ(capturedPkt->length() - capturedHdrSize, 210); /* TODO: why? */
    auto payload = capturedPkt->v6PayLoad()->payload()->payload();
    EXPECT_EQ(static_cast<PortID>(payload[0]), masterLogicalPortIds()[1]);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
