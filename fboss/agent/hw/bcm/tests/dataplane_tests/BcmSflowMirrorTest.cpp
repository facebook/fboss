// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using namespace ::testing;

DEFINE_int32(sflow_test_rate, 90000, "sflow sampling rate for bcm test");
DEFINE_int32(sflow_test_time, 5, "sflow test traffic time in seconds");

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

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  void
  configMirror(cfg::SwitchConfig* config, bool truncate, bool isV4 = true) {
    cfg::SflowTunnel sflowTunnel;
    *sflowTunnel.ip_ref() = isV4 ? "101.101.101.101" : "2401:101:101::101";
    sflowTunnel.udpSrcPort_ref() = 6545;
    sflowTunnel.udpDstPort_ref() = 5343;

    cfg::MirrorTunnel tunnel;
    tunnel.sflowTunnel_ref() = sflowTunnel;

    cfg::MirrorDestination destination;
    destination.tunnel_ref() = tunnel;

    config->mirrors_ref()->resize(1);
    *config->mirrors_ref()[0].name_ref() = "mirror";
    *config->mirrors_ref()[0].destination_ref() = destination;
    *config->mirrors_ref()[0].truncate_ref() = truncate;
  }

  void configSampling(cfg::SwitchConfig* config, int sampleRate) const {
    for (auto i = 1; i < masterLogicalPortIds().size(); i++) {
      auto portId = masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(*config, portId);
      *portCfg->sFlowIngressRate_ref() = sampleRate;
      portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
      portCfg->ingressMirror_ref() = "mirror";
    }
  }

  void configIngressMirrorOnPort(cfg::SwitchConfig* config, PortID port) const {
    for (auto i = 1; i < masterLogicalPortIds().size(); i++) {
      auto portId = masterLogicalPortIds()[i];
      auto portCfg = utility::findCfgPort(*config, portId);
      if (static_cast<PortID>(*portCfg->logicalID_ref()) == port) {
        portCfg->ingressMirror_ref() = "mirror";
        portCfg->sampleDest_ref() = cfg::SampleDestination::MIRROR;
        *portCfg->sFlowIngressRate_ref() = 1;
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

  void setupRoutes(bool disableTTL = true) {
    auto ports = masterLogicalPortIds();
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

    state = helper6.setupECMPForwarding(state, nhops);
    for (auto i = 1; i < size; i++) {
      boost::container::flat_set<PortDescriptor> port;
      port.insert(PortDescriptor(ports[i]));
      state = helper6.setupECMPForwarding(
          state,
          {PortDescriptor(ports[i])},
          {RouteV6::Prefix{
              folly::IPAddressV6(folly::to<std::string>(kIpStr, i, "::0")),
              80}});
      state = applyNewState(state);
    }
  }

  uint64_t getSampleCount(const std::map<PortID, HwPortStats>& stats) {
    auto portStats = stats.at(masterLogicalPortIds()[0]);
    return *portStats.outUnicastPkts__ref();
  }

  uint64_t getExpectedSampleCount(const std::map<PortID, HwPortStats>& stats) {
    uint64_t expectedSampleCount = 0;
    uint64_t allPortRx = 0;
    for (auto i = 1; i < masterLogicalPortIds().size(); i++) {
      auto port = masterLogicalPortIds()[i];
      auto portStats = stats.at(port);
      allPortRx += *portStats.inUnicastPkts__ref();
      expectedSampleCount +=
          (*portStats.inUnicastPkts__ref() / FLAGS_sflow_test_rate);
    }
    XLOG(INFO) << "total packets rx " << allPortRx;
    return expectedSampleCount;
  }

  void runSampleRateTest(size_t payloadSize = kDefaultPayloadSize) {
    if (!getPlatform()->sflowSamplingSupported()) {
      return;
    }
    if (payloadSize > kDefaultPayloadSize &&
        !getPlatform()->mirrorPktTruncationSupported()) {
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
      auto ports = masterLogicalPortIds();
      bringDownPorts(std::vector<PortID>(ports.begin() + 1, ports.end()));
      auto stats = getLatestPortStats(masterLogicalPortIds());
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

TEST_F(BcmSflowMirrorTest, VerifySampledPacketCount) {
  runSampleRateTest();
}

TEST_F(BcmSflowMirrorTest, VerifySampledPacketCountWithLargePackets) {
  runSampleRateTest(8192);
}
} // namespace facebook::fboss
