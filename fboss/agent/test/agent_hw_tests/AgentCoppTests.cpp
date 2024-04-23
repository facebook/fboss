/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/packet/DHCPv4Packet.h"
#include "fboss/agent/packet/DHCPv6Packet.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/types.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

DECLARE_bool(sai_user_defined_trap);

namespace {
constexpr auto kGetQueueOutPktsRetryTimes = 5;
/**
 * Link-local multicast network
 */
const auto kIPv6LinkLocalMcastAbsoluteAddress = folly::IPAddressV6("ff02::0");
const auto kIPv6LinkLocalMcastAddress = folly::IPAddressV6("ff02::5");
// Link-local unicast network
const auto kIPv6LinkLocalUcastAddress = folly::IPAddressV6("fe80::2");

const auto kMcastMacAddress = folly::MacAddress("01:05:0E:01:02:03");

const auto kNetworkControlDscp = 48;
const auto kRandomPort = 54131;

const auto kDhcpV6AllRoutersIp = folly::IPAddressV6("ff02::1:2");
const auto kDhcpV6McastMacAddress = folly::MacAddress("33:33:00:01:00:02");
const auto kDhcpV6ServerGlobalUnicastAddress =
    folly::IPAddressV6("2401:db00:eef0:a67::1");

static time_t getCurrentTime() {
  return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

using TestTypes =
    ::testing::Types<facebook::fboss::PortID, facebook::fboss::AggregatePortID>;
} // unnamed namespace

namespace facebook::fboss {

template <typename TestType>
class AgentCoppTest : public AgentHwTest {
  void setCmdLineFlagOverrides() const override {
    FLAGS_sai_user_defined_trap = true;
    AgentHwTest::setCmdLineFlagOverrides();
  }

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if constexpr (std::is_same_v<TestType, PortID>) {
      return {production_features::ProductionFeature::COPP};
    } else {
      return {
          production_features::ProductionFeature::COPP,
          production_features::ProductionFeature::LAG};
    }
  }

 protected:
  static constexpr auto isTrunk = std::is_same_v<TestType, AggregatePortID>;

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    if (isTrunk) {
      return getTrunkInitialConfig(ensemble);
    }

    auto asic = utility::getFirstAsic(ensemble.getSw());
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);

    utility::addOlympicQosMaps(cfg, asic);
    utility::setDefaultCpuTrafficPolicyConfig(cfg, asic, ensemble.isSai());
    utility::addCpuQueueConfig(cfg, asic, ensemble.isSai());
    return cfg;
  }

  cfg::SwitchConfig getTrunkInitialConfig(const AgentEnsemble& ensemble) const {
    auto asic = utility::getFirstAsic(ensemble.getSw());
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw()->getPlatformMapping(),
        asic,
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1],
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, asic, ensemble.isSai());
    utility::addCpuQueueConfig(cfg, asic, ensemble.isSai());
    utility::addAggPort(
        1,
        {ensemble.masterLogicalPortIds()[0],
         ensemble.masterLogicalPortIds()[1]},
        &cfg);
    return cfg;
  }

  void setup() {
    // COPP is part of initial config already
    if constexpr (isTrunk) {
      applyNewState([this](std::shared_ptr<SwitchState> /*in*/) {
        return utility::enableTrunkPorts(getProgrammedState());
      });
    }
  }

  folly::IPAddress getInSubnetNonSwitchIP() const {
    auto configIntf = initialConfig(*getAgentEnsemble()).interfaces()[0];
    auto ipAddress = configIntf.ipAddresses()[0];
    return folly::IPAddress::createNetwork(ipAddress, -1, true).first;
  }

  void sendTcpPkts(
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    utility::sendTcpPkts(
        getSw(),
        numPktsToSend,
        vlanId,
        dstMac ? *dstMac : intfMac,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0],
        trafficClass,
        payload);
  }

  // With onePortPerInterfaceConfig, we have a large (~200) number of
  // interfaces. Thus, if we send packet for every interface, the test takes
  // really long (5+ mins) to complete), and does not really offer additional
  // coverage. Thus, pick one IPv4 and IPv6 address and test.
  std::vector<std::string> getIpAddrsToSendPktsTo() const {
    std::set<std::string> ips;
    auto addV4AndV6 = [&](const auto& ipAddrs) {
      auto ipv4Addr =
          std::find_if(ipAddrs.begin(), ipAddrs.end(), [](const auto& ipAddr) {
            auto ip = folly::IPAddress::createNetwork(ipAddr, -1, false).first;
            return ip.isV4();
          });
      auto ipv6Addr =
          std::find_if(ipAddrs.begin(), ipAddrs.end(), [](const auto& ipAddr) {
            auto ip = folly::IPAddress::createNetwork(ipAddr, -1, false).first;
            return ip.isV6();
          });
      ips.insert(*ipv4Addr);
      ips.insert(*ipv6Addr);
    };
    addV4AndV6(*(this->initialConfig(*getAgentEnsemble())
                     .interfaces()[0]
                     .ipAddresses()));

    auto switchId = utility::getFirstSwitchId(getAgentEnsemble()->getSw());
    if (switchId) {
      auto dsfNode = getProgrammedState()->getDsfNodes()->getNodeIf(switchId);
      if (dsfNode) {
        auto loopbackIps = dsfNode->getLoopbackIps();
        std::vector<std::string> subnets;
        std::for_each(
            loopbackIps->begin(), loopbackIps->end(), [&](const auto& ip) {
              subnets.push_back(**ip);
            });
        addV4AndV6(subnets);
      }
    }

    return std::vector<std::string>{ips.begin(), ips.end()};
  }

  void sendPkt(
      std::unique_ptr<TxPacket> pkt,
      bool outOfPort,
      bool snoopAndVerify = false) {
    XLOG(DBG2) << "Packet Dump::"
               << folly::hexDump(pkt->buf()->data(), pkt->buf()->length());

    auto ethFrame = utility::makeEthFrame(*pkt, true /*skipTtlDecrement*/);
    utility::SwSwitchPacketSnooper snooper(
        getSw(), "snoop", std::nullopt, ethFrame);
    if (outOfPort) {
      getSw()->sendPacketOutOfPortAsync(
          std::move(pkt),
          masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(pkt));
    }
    if (snoopAndVerify) {
      WITH_RETRIES({
        auto frameRx = snooper.waitForPacket(1);
        EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
      });
    }
  }

  void sendTcpPktAndVerifyCpuQueue(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      std::optional<std::vector<uint8_t>> payload = std::nullopt,
      bool expectQueueHit = true,
      bool outOfPort = true) {
    const auto kNumPktsToSend = 1;
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto destinationMac =
        dstMac.value_or(utility::getFirstInterfaceMac(getProgrammedState()));
    auto sendAndInspect = [=, this]() {
      auto pkt = utility::makeTCPTxPacket(
          getSw(),
          vlanId,
          destinationMac,
          dstIpAddress,
          l4SrcPort,
          l4DstPort,
          trafficClass,
          payload);
      sendPkt(std::move(pkt), outOfPort, expectQueueHit /*snoopAndVerify*/);
    };
    utility::sendPktAndVerifyCpuQueue(
        getSw(),
        utility::getFirstSwitchId(getSw()),
        queueId,
        sendAndInspect,
        expectQueueHit ? kNumPktsToSend : 0);
  }

  void sendUdpPkt(
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort,
      uint8_t ttl,
      bool outOfPort,
      bool expectPktTrap) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    // arbit
    const auto srcIp =
        folly::IPAddress(dstIpAddress.isV4() ? "1.1.1.2" : "1::10");
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        srcIp,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        0 /* dscp */,
        ttl);

    XLOG(DBG2) << "UDP packet Dump::"
               << folly::hexDump(
                      txPacket->buf()->data(), txPacket->buf()->length());
    sendPkt(std::move(txPacket), outOfPort, expectPktTrap /*snoopAndVerify*/);
  }

  void sendUdpPktAndVerify(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      bool expectPktTrap = true,
      const int ttl = 255,
      bool outOfPort = false) {
    auto beforeOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    auto expectedPktDelta = expectPktTrap ? 1 : 0;
    sendUdpPkt(
        dstIpAddress, l4SrcPort, l4DstPort, ttl, outOfPort, expectPktTrap);
    auto afterOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        kGetQueueOutPktsRetryTimes,
        beforeOutPkts + 1);
    XLOG(DBG0) << "Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendEthPkts(
      int numPktsToSend,
      facebook::fboss::ETHERTYPE etherType,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());

    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeEthTxPacket(
          getSw(),
          vlanId,
          intfMac,
          dstMac ? *dstMac : intfMac,
          etherType,
          payload);
      sendPkt(std::move(txPacket), true /*outOfPort*/, true /*snoopAndVerify*/);
    }
  }

  void sendPktAndVerifyEthPacketsCpuQueue(
      int queueId,
      facebook::fboss::ETHERTYPE etherType,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt) {
    auto beforeOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    static auto payload = std::vector<uint8_t>(256, 0xff);
    payload[0] = 0x1; // sub-version of lacp packet
    sendEthPkts(1, etherType, dstMac, payload);
    auto afterOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        kGetQueueOutPktsRetryTimes,
        beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of dstMac="
               << (dstMac ? (*dstMac).toString()
                          : getLocalMacAddress().toString())
               << ". Ethertype=" << std::hex << int(etherType)
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(1, afterOutPkts - beforeOutPkts);
  }

  void setupEcmp() {
    if constexpr (!isTrunk) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), getLocalMacAddress());
      resolveNeigborAndProgramRoutes(ecmpHelper, 1);
    } else {
      utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
      flat_set<PortDescriptor> ports;
      ports.insert(PortDescriptor(AggregatePortID(1)));
      applyNewState(
          [this, &ports, &ecmpHelper](std::shared_ptr<SwitchState> /*in*/) {
            return ecmpHelper.resolveNextHops(getProgrammedState(), ports);
          });
      auto wrapper = getSw()->getRouteUpdater();
      ecmpHelper.programRoutes(&wrapper, ports);
    }
  }

  void sendArpPkts(
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      ARP_OPER arpType,
      bool outOfPort) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeARPTxPacket(
          getSw(),
          vlanId,
          srcMac,
          arpType == ARP_OPER::ARP_OPER_REQUEST
              ? folly::MacAddress("ff:ff:ff:ff:ff:ff")
              : intfMac,
          folly::IPAddress("1.1.1.2"),
          dstIpAddress,
          arpType);
      sendPkt(std::move(txPacket), outOfPort);
    }
  }

  void sendPktAndVerifyArpPacketsCpuQueue(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      ARP_OPER arpType,
      bool outOfPort = true,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    sendArpPkts(numPktsToSend, dstIpAddress, arpType, outOfPort);
    auto afterOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        kGetQueueOutPktsRetryTimes,
        beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of DstIp=" << dstIpAddress.str()
               << ", dstMac=" << ". Queue=" << queueId
               << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendNdpPkts(
      int numPktsToSend,
      const folly::IPAddressV6& neighborIp,
      ICMPv6Type type,
      bool outOfPort,
      bool selfSolicit) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto neighborMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket =
          (type == ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION)
          ? utility::makeNeighborSolicitation(
                getSw(),
                vlanId,
                selfSolicit ? neighborMac : intfMac, // solicitar mac
                neighborIp, // solicitar ip
                selfSolicit ? folly::IPAddressV6("1::1")
                            : folly::IPAddressV6("1::2")) // solicited address
          : utility::makeNeighborAdvertisement(
                getSw(),
                vlanId,
                neighborMac, // sender mac
                intfMac, // my mac
                neighborIp, // sender ip
                folly::IPAddressV6("1::")); // sent to me
      sendPkt(std::move(txPacket), outOfPort, true /*snoopAndVerify*/);
    }
  }

  void sendPktAndVerifyNdpPacketsCpuQueue(
      int queueId,
      const folly::IPAddressV6& neighborIp,
      ICMPv6Type ndpType,
      bool selfSolicit = true,
      bool outOfPort = true,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    sendNdpPkts(numPktsToSend, neighborIp, ndpType, outOfPort, selfSolicit);
    auto afterOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        kGetQueueOutPktsRetryTimes,
        beforeOutPkts + expectedPktDelta);
    XLOG(DBG0) << "Packet of neighbor=" << neighborIp.str()
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendPktAndVerifyLldpPacketsCpuQueue(
      int queueId,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto neighborMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    auto beforeOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeLLDPPacket(
          getSw(),
          neighborMac,
          vlanId,
          "rsw1dx.21.frc3",
          "eth1/1/1",
          "fsw001.p023.f01.frc3:eth4/9/1",
          LldpManager::TTL_TLV_VALUE,
          LldpManager::SYSTEM_CAPABILITY_ROUTER);
      getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), PortID(masterLogicalPortIds()[0]));
    }
    auto afterOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        kGetQueueOutPktsRetryTimes,
        beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of dstMac=" << LldpManager::LLDP_DEST_MAC.toString()
               << ". Ethertype=" << std::hex << int(LldpManager::ETHERTYPE_LLDP)
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void
  sendDHCPv6Pkts(int numPktsToSend, DHCPv6Type type, int ttl, bool outOfPort) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto neighborMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = (type == DHCPv6Type::DHCPv6_SOLICIT)
          ? utility::makeUDPTxPacket(
                getSw(),
                vlanId,
                neighborMac, // SrcMAC: Client's MAC address
                kDhcpV6McastMacAddress, // DstMac: 33:33:00:01:00:02
                kIPv6LinkLocalUcastAddress, // SrcIP: Client's Link Local addr
                kDhcpV6AllRoutersIp, // DstIP: ff02::1:2
                DHCPv6Packet::DHCP6_CLIENT_UDPPORT, // SrcPort: 546
                DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT, // DstPort: 547
                0 /* dscp */,
                ttl)
          : utility::makeUDPTxPacket( // DHCPv6Type::DHCPv6_ADVERTISE
                getSw(),
                vlanId,
                neighborMac, // srcMac: Server's MAC
                intfMac, // dstMac: Switch/our MAC
                kDhcpV6ServerGlobalUnicastAddress, // srcIp: Server's global
                                                   // unicast address
                folly::IPAddressV6("1::"), // dstIp: Switch/our IP
                kRandomPort, // SrcPort:DHCPv6 server's random port
                DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT, // DstPort: 547
                0 /* dscp */,
                ttl); // sent to me
      sendPkt(std::move(txPacket), outOfPort, true /* snoopAndVerify*/);
    }
  }

  void sendPktAndVerifyDHCPv6PacketsCpuQueue(
      int queueId,
      DHCPv6Type dhcpType,
      const int ttl = 1,
      bool outOfPort = true,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    sendDHCPv6Pkts(numPktsToSend, dhcpType, ttl, outOfPort);
    auto afterOutPkts = utility::getQueueOutPacketsWithRetry(
        getSw(),
        scopeResolver()
            .scope(masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        queueId,
        kGetQueueOutPktsRetryTimes,
        beforeOutPkts + expectedPktDelta);
    auto msgType =
        dhcpType == DHCPv6Type::DHCPv6_SOLICIT ? "SOLICIT" : "ADVERTISEMENT";
    XLOG(DBG0) << "DHCPv6 " << msgType << " packet" << ". Queue=" << queueId
               << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }
};

TYPED_TEST_SUITE(AgentCoppTest, TestTypes);

TYPED_TEST(AgentCoppTest, VerifyCoppPpsLowPri) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    auto kNumPktsToSend = 60000;
    auto kMinDurationInSecs = 12;
    const double kVariance = 0.30; // i.e. + or -30%

    auto beforeOutPkts = utility::getQueueOutPacketsWithRetry(
        this->getSw(),
        this->scopeResolver()
            .scope(
                this->masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        utility::kCoppLowPriQueueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);

    auto dstIP = this->getInSubnetNonSwitchIP();
    uint64_t afterSecs;
    /*
     * To avoid noise, the send traffic for at least kMinDurationInSecs.
     * In practice, sending kNumPktsToSend packets takes longer than
     * kMinDurationInSecs. But to be safe, keep sending packets for
     * at least kMinDurationInSecs.
     */
    auto beforeSecs = getCurrentTime();
    do {
      this->sendTcpPkts(
          kNumPktsToSend,
          dstIP,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);
      afterSecs = getCurrentTime();
    } while (afterSecs - beforeSecs < kMinDurationInSecs);

    auto afterOutPkts = utility::getQueueOutPacketsWithRetry(
        this->getSw(),
        this->scopeResolver()
            .scope(
                this->masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0])
            .switchId(),
        utility::kCoppLowPriQueueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    auto totalRecvdPkts = afterOutPkts - beforeOutPkts;
    auto duration = afterSecs - beforeSecs;
    auto currPktsPerSec = totalRecvdPkts / duration;
    uint32_t lowPriorityPps = utility::getCoppQueuePps(
        utility::getFirstAsic(this->getSw()), utility::kCoppLowPriQueueId);
    auto lowPktsPerSec = lowPriorityPps * (1 - kVariance);
    auto highPktsPerSec = lowPriorityPps * (1 + kVariance);

    XLOG(DBG0) << "Before pkts: " << beforeOutPkts
               << " after pkts: " << afterOutPkts
               << " totalRecvdPkts: " << totalRecvdPkts
               << " duration: " << duration
               << " currPktsPerSec: " << currPktsPerSec
               << " low pktsPerSec: " << lowPktsPerSec
               << " high pktsPerSec: " << highPktsPerSec;

    /*
     * In practice, if no pps is configured, using the above method, the
     * packets are received at a rate > 2500 per second.
     */
    EXPECT_TRUE(
        lowPktsPerSec <= currPktsPerSec && currPktsPerSec <= highPktsPerSec);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, LocalDstIpBgpPortToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    // Make sure all dstip(=interfaces local ips) + BGP port packets send to
    // cpu high priority queue
    enum SRC_DST { SRC, DST };
    auto asic = utility::getFirstAsic(this->getSw());
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      for (int dir = 0; dir <= DST; dir++) {
        XLOG(DBG2) << "Send Pkt to: " << ipAddress
                   << " dir: " << (dir == DST ? " DST" : " SRC");
        this->sendTcpPktAndVerifyCpuQueue(
            utility::getCoppHighPriQueueId(asic),
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            dir == SRC ? utility::kBgpPort : utility::kNonSpecialPort1,
            dir == DST ? utility::kBgpPort : utility::kNonSpecialPort1);
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, LocalDstIpNonBgpPortToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          utility::getQueueOutPacketsWithRetry(
              this->getSw(),
              this->scopeResolver()
                  .scope(this->masterLogicalPortIds(
                      {cfg::PortType::INTERFACE_PORT})[0])
                  .switchId(),
              utility::getCoppHighPriQueueId(
                  utility::getFirstAsic(this->getSw())),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalMcastToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          kMcastMacAddress);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          utility::getQueueOutPacketsWithRetry(
              this->getSw(),
              this->scopeResolver()
                  .scope(this->masterLogicalPortIds(
                      {cfg::PortType::INTERFACE_PORT})[0])
                  .switchId(),
              utility::getCoppHighPriQueueId(
                  utility::getFirstAsic(this->getSw())),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalMcastTxFromCpu) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    // Intent of this test is to verify that
    // link local ipv6 address is not looped back when sent from CPU
    this->sendUdpPktAndVerify(
        utility::kCoppMidPriQueueId,
        folly::IPAddressV6("ff02::1"),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        false /* expectPktTrap */);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalUcastToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    // Device link local unicast address should use mid-pri queue
    {
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, getLocalMacAddress());

      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          utility::getQueueOutPacketsWithRetry(
              this->getSw(),
              this->scopeResolver()
                  .scope(this->masterLogicalPortIds(
                      {cfg::PortType::INTERFACE_PORT})[0])
                  .switchId(),
              utility::getCoppHighPriQueueId(
                  utility::getFirstAsic(this->getSw())),
              kGetQueueOutPktsRetryTimes,
              0));
    }
    // Non device link local unicast address should also use mid-pri queue
    {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          utility::getQueueOutPacketsWithRetry(
              this->getSw(),
              this->scopeResolver()
                  .scope(this->masterLogicalPortIds(
                      {cfg::PortType::INTERFACE_PORT})[0])
                  .switchId(),
              utility::getCoppHighPriQueueId(
                  utility::getFirstAsic(this->getSw())),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, SlowProtocolsMacToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    this->sendPktAndVerifyEthPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        facebook::fboss::ETHERTYPE::ETHERTYPE_SLOW_PROTOCOLS,
        LACPDU::kSlowProtocolsDstMac());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, EapolToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    this->sendPktAndVerifyEthPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        facebook::fboss::ETHERTYPE::ETHERTYPE_EAPOL,
        folly::MacAddress("ff:ff:ff:ff:ff:ff"));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, DstIpNetworkControlDscpToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
    // Non local dst ip with kNetworkControlDscp should not hit high pri queue
    // (since it won't even trap to cpu)
    this->sendTcpPktAndVerifyCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        folly::IPAddress("2::2"),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt,
        kNetworkControlDscp,
        std::nullopt,
        false /*expectQueueHit*/);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalUcastIpNetworkControlDscpToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    // Device link local unicast address + kNetworkControlDscp should use
    // high-pri queue
    {
      XLOG(DBG2) << "send device link local packet";
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, utility::kLocalCpuMac());

      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
    // Non device link local unicast address + kNetworkControlDscp dscp should
    // also use high-pri queue
    {
      XLOG(DBG2) << "send non-device link local packet";
      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

/*
 * Testcase to test that link local ucast packets from cpu port does not get
 * copied back to cpu. The test does the following
 * 1. copp acl to match link local ucast address and cpu srcPort is created as a
 * part of setup
 * 2. Sends a link local unicast packet through CPU PIPELINE_LOOKUP.
 * 3. Packet hits the newly created acl(and so does not get forwarded to cpu).
 * It goes out through the front port and loops back in.
 * 4. On getting received, the packet bypasses the newly created acl(since src
 * port is no longer cpu port) and hits the acl for regular link local ucast
 * packets. This gets forwarded to cpu queue 9
 */
TYPED_TEST(AgentCoppTest, CpuPortIpv6LinkLocalUcastIp) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    auto nbrLinkLocalAddr = folly::IPAddressV6("fe80:face:b11c::1");
    auto randomMac = folly::MacAddress("00:00:00:00:00:01");
    this->sendTcpPktAndVerifyCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        nbrLinkLocalAddr,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        randomMac,
        kNetworkControlDscp,
        std::nullopt,
        true,
        false /*outOfPort*/);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ipv6LinkLocalMcastNetworkControlDscpToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          kMcastMacAddress,
          kNetworkControlDscp);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, L3MTUErrorToLowPriQ) {
  auto setup = [=, this]() {
    this->setup();
    this->setupEcmp();
  };
  auto verify = [=, this]() {
    // Make sure all packets packet with large payload (> MTU)
    // are sent to cpu low priority queue.
    // Port Max Frame size is set to 9412 and L3 MTU is set as 9000
    // Thus sending a packet sized between 9000 and 9412 to cause the trap.
    auto randomIP = folly::IPAddressV6("2::2");
    this->sendTcpPktAndVerifyCpuQueue(
        utility::kCoppLowPriQueueId,
        randomIP,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt,
        0, /* traffic class*/
        std::vector<uint8_t>(9200, 0xff));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, ArpRequestAndReplyToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };
  auto verify = [=, this]() {
    this->sendPktAndVerifyArpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        folly::IPAddressV4("1.1.1.5"),
        ARP_OPER::ARP_OPER_REQUEST);
    this->sendPktAndVerifyArpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        folly::IPAddressV4("1.1.1.5"),
        ARP_OPER::ARP_OPER_REPLY);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, NdpSolicitationToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };
  auto verify = [=, this]() {
    XLOG(DBG2) << "verifying solicitation";
    this->sendPktAndVerifyNdpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        folly::IPAddressV6("1::2"), // sender of solicitation
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, NdpSolicitNeighbor) {
  // This test case makes sure that addNoActionAclForNw() is
  // present in ACL table and at the higher priority than that of
  // ACL entry to trap NDP solicit to high priority queue.
  // If both ACL entries are present in the ACL table and at the right order
  // we would recive 1 packet to CPU.
  // Reason: NS packet enters ASIC via CPU port, hits addNoActionAclForNw() ACL
  // because, prefix macthes ff02::/16 and source port is set to CPU port. Hence
  // it will be forwarded. Since it's a multicast packet, it will be sent out of
  // port 1, and the packet loops back into ASIC via port 1.
  //  Now, it hits the NDP solicit ACL entry and copied to CPU.
  // Note that it did NOT hit addNoActionAclForNw() because source port is set
  // to 1 after looping back.

  // If not we would receive 2 packets to CPU.
  // Reason: When packet enters the ASIC via CPU port, it hits the
  // addNoActionAclForNw() and it's copied to CPU and another copy is sent out
  // of port 1. Now, since we have port 1 in loopback mode, packet enters back
  // into ASIC via port 1, addNoActionAclForNw() hits  again and copied to CPU
  // again.

  // More explanation in the test plan section of - D34782575
  auto setup = [=, this]() { this->setup(); };
  auto verify = [=, this]() {
    XLOG(DBG2) << "verifying solicitation";
    this->sendPktAndVerifyNdpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        folly::IPAddressV6("1::1"), // sender of solicitation
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
        false,
        false,
        1,
        1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, NdpAdvertisementToHighPriQ) {
  auto setup = [=, this]() { this->setup(); };
  auto verify = [=, this]() {
    XLOG(DBG2) << "verifying advertisement";
    this->sendPktAndVerifyNdpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
        folly::IPAddressV6("1::2"), // sender of advertisement
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, UnresolvedRoutesToLowPriQueue) {
  auto setup = [=, this]() {
    this->setup();
    utility::EcmpSetupAnyNPorts6 ecmp6(this->getProgrammedState());
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmp6.programRoutes(&wrapper, 1);
  };
  auto randomIP = folly::IPAddressV6("2::2");
  auto verify = [=, this]() {
    this->sendTcpPktAndVerifyCpuQueue(
        utility::kCoppLowPriQueueId,
        randomIP,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, UnresolvedRouteNextHopToLowPriQueue) {
  static const std::vector<RoutePrefixV6> routePrefixes = {
      RoutePrefix<folly::IPAddressV6>{
          folly::IPAddressV6{"2803:6080:d038:3063::"}, 64},
      RoutePrefix<folly::IPAddressV6>{
          folly::IPAddressV6{"2803:6080:d038:3065::1"}, 128}};
  auto setup = [=, this]() {
    FLAGS_classid_for_unresolved_routes = true;
    this->setup();
    utility::EcmpSetupAnyNPorts6 ecmp6(this->getProgrammedState());
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmp6.programRoutes(&wrapper, 1, routePrefixes);
  };
  // Different from UnresolvedRoutesToLowPriQueue as traffic is
  // destined to a remote route for which next hop is unresolved.
  const auto randomNonsubnetUnicastIpAddresses = {
      folly::IPAddressV6("2803:6080:d038:3063::1"),
      folly::IPAddressV6("2803:6080:d038:3065::1")};
  auto verify = [=, this]() {
    for (auto& randomNonsubnetUnicastIpAddress :
         randomNonsubnetUnicastIpAddresses) {
      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppLowPriQueueId,
          randomNonsubnetUnicastIpAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          0 /* trafficClass */,
          std::nullopt,
          true /* expectQueueHit */);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, JumboFramesToQueues) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    std::vector<uint8_t> jumboPayload(7000, 0xff);
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      // High pri queue
      this->sendTcpPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(utility::getFirstAsic(this->getSw())),
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kBgpPort,
          utility::kNonSpecialPort2,
          std::nullopt, /*mac*/
          0, /* traffic class*/
          jumboPayload);
      // Mid pri queue
      this->sendTcpPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt, /*mac*/
          0, /* traffic class*/
          jumboPayload);
    }
    this->sendTcpPktAndVerifyCpuQueue(
        utility::kCoppLowPriQueueId,
        this->getInSubnetNonSwitchIP(),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt, /*mac*/
        0, /* traffic class*/
        jumboPayload);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, LldpProtocolToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    this->sendPktAndVerifyLldpPacketsCpuQueue(utility::kCoppMidPriQueueId);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, Ttl1PacketToLowPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    auto randomIP = folly::IPAddressV6("2::2");
    this->sendUdpPktAndVerify(
        utility::kCoppLowPriQueueId,
        randomIP,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        true /* expectPktTrap */,
        1 /* TTL */,
        true /* send out of port */);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, DhcpPacketToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };

  auto verify = [=, this]() {
    std::array<folly::IPAddress, 2> randomSrcIP{
        folly::IPAddress("1.1.1.10"), folly::IPAddress("1::10")};
    std::array<std::pair<int, int>, 2> dhcpPortPairs{
        std::make_pair(67, 68), std::make_pair(546, 547)};
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
        auto l4SrcPort =
            j == 0 ? dhcpPortPairs[i].first : dhcpPortPairs[i].second;
        auto l4DstPort =
            j == 0 ? dhcpPortPairs[i].second : dhcpPortPairs[i].first;
        this->sendUdpPktAndVerify(
            utility::kCoppMidPriQueueId,
            randomSrcIP[i],
            l4SrcPort,
            l4DstPort,
            true /* expectPktTrap */,
            255 /* TTL */,
            true /* send out of port */);
        XLOG(DBG0) << "Sending packet with src port " << l4SrcPort
                   << " dst port " << l4DstPort << " IP: " << randomSrcIP[i];
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, DHCPv6SolicitToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };
  auto verify = [=, this]() {
    XLOG(DBG2) << "verifying DHCPv6 solicitation with TTL 1";
    this->sendPktAndVerifyDHCPv6PacketsCpuQueue(
        utility::kCoppMidPriQueueId, DHCPv6Type::DHCPv6_SOLICIT);
    XLOG(DBG2) << "verifying DHCPv6 solicitation with TTL 128";
    this->sendPktAndVerifyDHCPv6PacketsCpuQueue(
        utility::kCoppMidPriQueueId, DHCPv6Type::DHCPv6_SOLICIT, 128);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentCoppTest, DHCPv6AdvertiseToMidPriQ) {
  auto setup = [=, this]() { this->setup(); };
  auto verify = [=, this]() {
    XLOG(DBG2) << "verifying DHCPv6 Advertise with TTL 1";
    this->sendPktAndVerifyDHCPv6PacketsCpuQueue(
        utility::kCoppMidPriQueueId, DHCPv6Type::DHCPv6_ADVERTISE);
    XLOG(DBG2) << "verifying DHCPv6 Advertise with TTL 128";
    this->sendPktAndVerifyDHCPv6PacketsCpuQueue(
        utility::kCoppMidPriQueueId, DHCPv6Type::DHCPv6_ADVERTISE, 128);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
