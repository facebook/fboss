/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestTrunkUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/packet/DHCPv6Packet.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/types.h"
#include "folly/Utility.h"

#include <folly/IPAddress.h>
#include <folly/container/Array.h>

#include <gtest/gtest.h>

namespace {
constexpr auto kGetQueueOutPktsRetryTimes = 5;
/**
 * Link-local multicast network
 */
const auto kIPv6LinkLocalMcastAbsoluteAddress = folly::IPAddressV6("ff02::0");
const auto kIPv6LinkLocalMcastAddress = folly::IPAddressV6("ff02::5");
// Link-local unicast network
const auto kIPv6LinkLocalUcastAddress = folly::IPAddressV6("fe80::2");
constexpr uint8_t kNetworkControlDscp = 48;

const auto kMcastMacAddress = folly::MacAddress("01:05:0E:01:02:03");

const auto kDhcpV6AllRoutersIp = folly::IPAddressV6("ff02::1:2");
const auto kDhcpV6McastMacAddress = folly::MacAddress("33:33:00:01:00:02");
const auto kDhcpV6ServerGlobalUnicastAddress =
    folly::IPAddressV6("2401:db00:eef0:a67::1");
const auto kRandomPort = 54131;

static time_t getCurrentTime() {
  return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
}

using TestTypes =
    ::testing::Types<facebook::fboss::PortID, facebook::fboss::AggregatePortID>;
} // unnamed namespace

namespace facebook::fboss {

template <typename TestType>
class HwCoppTest : public HwLinkStateDependentTest {
 protected:
  static constexpr auto isTrunk = std::is_same_v<TestType, AggregatePortID>;

  cfg::SwitchConfig initialConfig() const override {
    if (isTrunk) {
      return getTrunkInitialConfig();
    }
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackMode(),
        true /*interfaceHasSubnet*/);
    utility::addOlympicQosMaps(cfg, getAsic());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    utility::addCpuQueueConfig(cfg, getAsic());
    return cfg;
  }

  cfg::SwitchConfig getTrunkInitialConfig() const {
    auto cfg = utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        getAsic()->desiredLoopbackMode());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, this->getAsic());
    utility::addCpuQueueConfig(cfg, this->getAsic());
    utility::addAggPort(
        1, {masterLogicalPortIds()[0], masterLogicalPortIds()[1]}, &cfg);
    return cfg;
  }

  void setup() {
    // COPP is part of initial config already
    if constexpr (isTrunk) {
      applyNewState(utility::enableTrunkPorts(getProgrammedState()));
    }
  }

  void setupEcmp() {
    if constexpr (!isTrunk) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), getPlatform()->getLocalMac());
      resolveNeigborAndProgramRoutes(ecmpHelper, 1);
    } else {
      utility::EcmpSetupTargetedPorts6 ecmpHelper(getProgrammedState());
      flat_set<PortDescriptor> ports;
      ports.insert(PortDescriptor(AggregatePortID(1)));
      applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), ports));
      ecmpHelper.programRoutes(getRouteUpdater(), ports);
    }
  }

  void sendPkt(std::unique_ptr<TxPacket> pkt, bool outOfPort) {
    XLOG(DBG2) << "Packet Dump::"
               << folly::hexDump(pkt->buf()->data(), pkt->buf()->length());

    if (outOfPort) {
      getHwSwitch()->sendPacketOutOfPortSync(
          std::move(pkt),
          masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})[0]);
    } else {
      getHwSwitch()->sendPacketSwitchedSync(std::move(pkt));
    }
  }

  void sendUdpPkts(
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort,
      uint8_t ttl,
      bool outOfPort) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    // arbit
    const auto srcIp =
        folly::IPAddress(dstIpAddress.isV4() ? "1.1.1.2" : "1::10");
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeUDPTxPacket(
          getHwSwitch(),
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

      sendPkt(std::move(txPacket), outOfPort);
    }
  }

  /*
  const auto  srcIp = folly::IPAddress("1::10");
  auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);
  */

  void sendEthPkts(
      int numPktsToSend,
      facebook::fboss::ETHERTYPE etherType,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());

    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeEthTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          dstMac ? *dstMac : intfMac,
          etherType,
          payload);
      sendPkt(std::move(txPacket), true);
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
          getHwSwitch(),
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
                getHwSwitch(),
                vlanId,
                selfSolicit ? neighborMac : intfMac, // solicitar mac
                neighborIp, // solicitar ip
                selfSolicit ? folly::IPAddressV6("1::1")
                            : folly::IPAddressV6("1::2")) // solicited address
          : utility::makeNeighborAdvertisement(
                getHwSwitch(),
                vlanId,
                neighborMac, // sender mac
                intfMac, // my mac
                neighborIp, // sender ip
                folly::IPAddressV6("1::1")); // sent to me
      sendPkt(std::move(txPacket), outOfPort);
    }
  }

  void
  sendDHCPv6Pkts(int numPktsToSend, DHCPv6Type type, int ttl, bool outOfPort) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto neighborMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = (type == DHCPv6Type::DHCPv6_SOLICIT)
          ? utility::makeUDPTxPacket(
                getHwSwitch(),
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
                getHwSwitch(),
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
      sendPkt(std::move(txPacket), outOfPort);
    }
  }

  uint64_t getQueueOutPacketsWithRetry(
      int queueId,
      int retryTimes,
      uint64_t expectedNumPkts,
      int postMatchRetryTimes = 2) {
    uint64_t outPkts = 0, outBytes = 0;
    do {
      std::tie(outPkts, outBytes) =
          utility::getCpuQueueOutPacketsAndBytes(getHwSwitch(), queueId);
      if (retryTimes == 0 || (outPkts >= expectedNumPkts)) {
        break;
      }

      /*
       * Post warmboot, the packet always gets processed by the right CPU
       * queue (as per ACL/rxreason etc.) but sometimes it is delayed.
       * Retrying a few times to avoid test noise.
       */
      XLOG(DBG0) << "Retry...";
      /* sleep override */
      sleep(1);
    } while (retryTimes-- > 0);

    while ((outPkts == expectedNumPkts) && postMatchRetryTimes--) {
      std::tie(outPkts, outBytes) =
          utility::getCpuQueueOutPacketsAndBytes(getHwSwitch(), queueId);
    }

    return outPkts;
  }

  void sendUdpPktAndVerify(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 0,
      const int ttl = 255,
      bool outOfPort = false) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    sendUdpPkts(
        numPktsToSend, dstIpAddress, l4SrcPort, l4DstPort, ttl, outOfPort);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
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
        getHwSwitch(),
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

  void sendPktAndVerifyCpuQueue(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto sendPkts = [=]() {
      sendTcpPkts(
          numPktsToSend,
          dstIpAddress,
          l4SrcPort,
          l4DstPort,
          dstMac,
          trafficClass,
          payload);
    };
    utility::sendPktAndVerifyCpuQueue(
        getHwSwitch(), queueId, sendPkts, expectedPktDelta);
  }

  void sendPktAndVerifyEthPacketsCpuQueue(
      int queueId,
      facebook::fboss::ETHERTYPE etherType,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    static auto payload = std::vector<uint8_t>(256, 0xff);
    payload[0] = 0x1; // sub-version of lacp packet
    sendEthPkts(numPktsToSend, etherType, dstMac, payload);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of dstMac="
               << (dstMac ? (*dstMac).toString()
                          : getPlatform()->getLocalMac().toString())
               << ". Ethertype=" << std::hex << int(etherType)
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendPktAndVerifyArpPacketsCpuQueue(
      int queueId,
      const folly::IPAddress& dstIpAddress,
      ARP_OPER arpType,
      bool outOfPort = true,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    sendArpPkts(numPktsToSend, dstIpAddress, arpType, outOfPort);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of DstIp=" << dstIpAddress.str() << ", dstMac="
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendPktAndVerifyNdpPacketsCpuQueue(
      int queueId,
      const folly::IPAddressV6& neighborIp,
      ICMPv6Type ndpType,
      bool selfSolicit = true,
      bool outOfPort = true,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    sendNdpPkts(numPktsToSend, neighborIp, ndpType, outOfPort, selfSolicit);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + expectedPktDelta);
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
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    for (int i = 0; i < numPktsToSend; i++) {
      auto txPacket = utility::makeLLDPPacket(
          getHwSwitch(),
          neighborMac,
          vlanId,
          "FBOSS",
          "rsw1dx.21.frc3",
          "eth1/1/1",
          "fsw001.p023.f01.frc3:eth4/9/1",
          LldpManager::TTL_TLV_VALUE,
          LldpManager::SYSTEM_CAPABILITY_ROUTER);
      getHwSwitch()->sendPacketOutOfPortSync(
          std::move(txPacket), PortID(masterLogicalPortIds()[0]));
    }
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + 1);
    XLOG(DBG0) << "Packet of dstMac=" << LldpManager::LLDP_DEST_MAC.toString()
               << ". Ethertype=" << std::hex << int(LldpManager::ETHERTYPE_LLDP)
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void sendPktAndVerifyDHCPv6PacketsCpuQueue(
      int queueId,
      DHCPv6Type dhcpType,
      const int ttl = 1,
      bool outOfPort = true,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1) {
    auto beforeOutPkts = getQueueOutPacketsWithRetry(
        queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    sendDHCPv6Pkts(numPktsToSend, dhcpType, ttl, outOfPort);
    auto afterOutPkts = getQueueOutPacketsWithRetry(
        queueId, kGetQueueOutPktsRetryTimes, beforeOutPkts + expectedPktDelta);
    auto msgType =
        dhcpType == DHCPv6Type::DHCPv6_SOLICIT ? "SOLICIT" : "ADVERTISEMENT";
    XLOG(DBG0) << "DHCPv6 " << msgType << " packet"
               << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  folly::IPAddress getInSubnetNonSwitchIP() const {
    auto configIntf = initialConfig().interfaces()[0];
    auto ipAddress = configIntf.ipAddresses()[0];
    return folly::IPAddress::createNetwork(ipAddress, -1, true).first;
  }

  // With onePortPerInterfaceConfig, we have a large (~200) number of
  // interfaces. Thus, if we send packet for every interface, the test takes
  // really long (5+ mins) to complete), and does not really offer additional
  // coverage. Thus, pick one IPv4 and IPv6 address and test.
  std::vector<std::string> getIpAddrsToSendPktsTo() const {
    auto ipAddrs = *(this->initialConfig().interfaces()[0].ipAddresses());
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

    return std::vector<std::string>{*ipv4Addr, *ipv6Addr};
  }

 private:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
};

class HwCoppQosTest : public HwLinkStateDependentTest {
  using pktReceivedCb = folly::Function<void(RxPacket* pkt) const>;

 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::twoL3IntfConfig(
        getHwSwitch(),
        masterLogicalInterfacePortIds()[0],
        masterLogicalInterfacePortIds()[1],
        getAsic()->desiredLoopbackMode());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    addCustomCpuQueueConfig(cfg, getAsic());
    return cfg;
  }

  void setupEcmpDataplaneLoop() {
    auto dstMac = utility::getFirstInterfaceMac(initialConfig());

    utility::EcmpSetupAnyNPorts6 ecmpHelper(getProgrammedState(), dstMac);
    resolveNeigborAndProgramRoutes(ecmpHelper, 1);
    auto& nextHop = ecmpHelper.getNextHops()[0];
    utility::disableTTLDecrements(
        getHwSwitch(), ecmpHelper.getRouterId(), nextHop);
  }

  void packetReceived(RxPacket* pkt) noexcept override {
    auto receivedCallback = pktReceivedCallback_.lock();
    if (*receivedCallback) {
      (*receivedCallback)(pkt);
    }
  }

  void registerPktReceivedCallback(pktReceivedCb callback) {
    *pktReceivedCallback_.lock() = std::move(callback);
  }

  void unRegisterPktReceivedCallback() {
    *pktReceivedCallback_.lock() = nullptr;
  }

  std::optional<folly::IPAddress> getDestinationIpIfValid(RxPacket* pkt) {
    std::optional<folly::IPAddress> destinationAddress;
    auto pktData = pkt->buf()->clone();
    folly::io::Cursor cursor{pktData.get()};
    auto receivedPkt = utility::EthFrame{cursor};
    auto v6Packet = receivedPkt.v6PayLoad();
    if (v6Packet.has_value()) {
      auto v6Header = v6Packet->header();
      destinationAddress = v6Header.dstAddr;
    } else {
      auto v4Packet = receivedPkt.v4PayLoad();
      if (v4Packet.has_value()) {
        auto v4Header = v4Packet->header();
        destinationAddress = v4Header.dstAddr;
      }
    }

    return destinationAddress;
  }

  void sendTcpPktsOnPort(
      PortID port,
      std::optional<VlanID> vlanId,
      int numPktsToSend,
      const folly::IPAddress& dstIpAddress,
      int l4SrcPort,
      int l4DstPort,
      const std::optional<folly::MacAddress>& dstMac = std::nullopt,
      uint8_t trafficClass = 0,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) {
    auto intfMac = utility::getFirstInterfaceMac(initialConfig());
    utility::sendTcpPkts(
        getHwSwitch(),
        numPktsToSend,
        vlanId,
        dstMac ? *dstMac : intfMac,
        dstIpAddress,
        l4SrcPort,
        l4DstPort,
        port,
        trafficClass,
        payload);
  }

  void createLineRateTrafficOnPort(
      PortID port,
      std::optional<VlanID> vlanId,
      const folly::IPAddress& dstIpAddress) {
    auto minPktsForLineRate =
        getHwSwitchEnsemble()->getMinPktsForLineRate(port);
    auto dstMac = utility::getFirstInterfaceMac(initialConfig());

    // Create a loop with specified destination packets.
    // We want to send atleast 2 traffic streams to ensure we dont run
    // into throughput limits with single flow and flow cache for TAJO.
    sendTcpPktsOnPort(
        port,
        vlanId,
        minPktsForLineRate / 2,
        dstIpAddress,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        dstMac);
    sendTcpPktsOnPort(
        port,
        vlanId,
        minPktsForLineRate / 2,
        dstIpAddress,
        utility::kNonSpecialPort1 + 1,
        utility::kNonSpecialPort2 + 1,
        dstMac);
    std::string vlanStr = (vlanId ? folly::to<std::string>(*vlanId) : "None");
    XLOG(DBG0) << "Sent " << minPktsForLineRate << " TCP packets on port "
               << (int)port << " / VLAN " << vlanStr;

    // Wait for packet loop buildup
    getHwSwitchEnsemble()->waitForLineRateOnPort(port);
    XLOG(DBG0) << "Created dataplane loop with packets for "
               << dstIpAddress.str();
  }

  /*
   * Send packets as fast as possible from CPU in groups of
   * packetsPerBurst every second. Once these many packets
   * are sent, sleep for rest of the time (if any) in that
   * second. This helps ensure that for a burst of packets
   * that can be sent in a second, we dont exceed the number
   * of packets sent in each second.
   */
  void sendPacketBursts(
      const PortID& port,
      std::optional<VlanID> vlanId,
      const int packetCount,
      const int packetsPerBurst,
      const folly::IPAddress& dstIpAddress,
      const int l4SrcPort,
      const int l4DstPort) {
    double waitTimeMsec = 1000;
    int packetsSent = 0;
    uint64_t hiPriWatermarkBytes{};
    uint64_t hiPriorityCoppQueueDiscardStats{};

    while (packetsSent < packetCount) {
      auto loopStartTime(std::chrono::steady_clock::now());
      auto pktsInThisBurst = (packetCount - packetsSent > packetsPerBurst)
          ? packetsPerBurst
          : packetCount - packetsSent;
      sendTcpPktsOnPort(
          port, vlanId, pktsInThisBurst, dstIpAddress, l4SrcPort, l4DstPort);
      packetsSent += pktsInThisBurst;

      std::chrono::duration<double, std::milli> msecUsed =
          std::chrono::steady_clock::now() - loopStartTime;
      if (waitTimeMsec > msecUsed.count()) {
        auto remainingTimeMsec = waitTimeMsec - msecUsed.count();
        std::this_thread::sleep_for(
            std::chrono::milliseconds(static_cast<long>(remainingTimeMsec)));
      } else {
        XLOG(WARN) << "Not sleeping between bursts as time taken to send "
                   << msecUsed.count() << "msec is higher than " << waitTimeMsec
                   << "msec";
      }

      /*
       * XXX: Debug code to be removed later.
       * Keep checking the low and high priority watermarks for
       * CPU queues, printing for now to help analyze the drops
       * happening in HW!
       */
      auto watermarkStats = utility::getCpuQueueWatermarkStats(getHwSwitch());
      auto loPriWatermarkBytes_1 = utility::getCpuQueueWatermarkBytes(
          watermarkStats, utility::kCoppLowPriQueueId);
      auto hiPriWatermarkBytes_1 = utility::getCpuQueueWatermarkBytes(
          watermarkStats, utility::getCoppHighPriQueueId(getAsic()));

      auto hiPriorityCoppQueueDiscardStats_1 =
          utility::getCpuQueueOutDiscardPacketsAndBytes(
              getHwSwitch(), utility::getCoppHighPriQueueId(getAsic()))
              .first;
      if (hiPriWatermarkBytes_1 != hiPriWatermarkBytes ||
          hiPriorityCoppQueueDiscardStats_1 !=
              hiPriorityCoppQueueDiscardStats) {
        XLOG(DBG0) << "HiPri watermark: " << hiPriWatermarkBytes_1
                   << ", LoPri watermark: " << loPriWatermarkBytes_1
                   << ", HiPri Drops: " << hiPriorityCoppQueueDiscardStats_1;
        hiPriWatermarkBytes = hiPriWatermarkBytes_1;
        hiPriorityCoppQueueDiscardStats = hiPriorityCoppQueueDiscardStats_1;
      }
    }
    std::string vlanStr = (vlanId ? folly::to<std::string>(*vlanId) : "None");
    XLOG(DBG0) << "Sent " << packetCount << " TCP packets on port " << (int)port
               << " / VLAN " << vlanStr << " in bursts of " << packetsPerBurst
               << " packets";
  }

  /*
   * addCustomCpuQueueConfig() is a modified version of
   * utility::addCpuQueueConfig(), differences:
   *   1) CPU low pri queue is given the same weight as mid pri queue
   *   2) No rate limiting on queues
   *   3) Optional param to specify ECN config on queue
   * The objective of the config is to make sure low priority queue
   * behaves just like the mid priority queue. Test here tries to
   * validate prioritization of CPU high priority queue traffic vs
   * other priority traffic. To ensure there is maximum traffic in
   * the lower priority queue, dataplane loop is created on one of the
   * port and the same traffic copied to CPU. As there is not enough
   * support to copy traffic from data plane to mid priority queue
   * for all platforms, copying this traffic to low priority queue
   * and removing the rate limiting on low priority CPU queue.
   */
  void addCustomCpuQueueConfig(
      cfg::SwitchConfig& config,
      const HwAsic* hwAsic,
      bool addEcnConfig = false) const {
    std::vector<cfg::PortQueue> cpuQueues;

    cfg::PortQueue queue0;
    queue0.id() = utility::kCoppLowPriQueueId;
    queue0.name() = "cpuQueue-low";
    queue0.streamType() = utility::getCpuDefaultStreamType(hwAsic);
    queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue0.weight() =
        utility::kCoppMidPriWeight; // Weight of mid priority queue
    if (addEcnConfig) {
      queue0.aqms() = {};
      queue0.aqms()->push_back(utility::kGetOlympicEcnConfig());
    }
    cpuQueues.push_back(queue0);

    cfg::PortQueue queue1;
    queue1.id() = utility::kCoppDefaultPriQueueId;
    queue1.name() = "cpuQueue-default";
    queue1.streamType() = utility::getCpuDefaultStreamType(hwAsic);
    queue1.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue1.weight() = utility::kCoppDefaultPriWeight;
    if (addEcnConfig) {
      queue1.aqms() = {};
      queue1.aqms()->push_back(utility::kGetOlympicEcnConfig());
    }
    cpuQueues.push_back(queue1);

    cfg::PortQueue queue2;
    queue2.id() = utility::kCoppMidPriQueueId;
    queue2.name() = "cpuQueue-mid";
    queue2.streamType() = utility::getCpuDefaultStreamType(hwAsic);
    queue2.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue2.weight() = utility::kCoppMidPriWeight;
    if (addEcnConfig) {
      queue2.aqms() = {};
      queue2.aqms()->push_back(utility::kGetOlympicEcnConfig());
    }
    cpuQueues.push_back(queue2);

    cfg::PortQueue queue9;
    queue9.id() = utility::getCoppHighPriQueueId(hwAsic);
    queue9.name() = "cpuQueue-high";
    queue9.streamType() = utility::getCpuDefaultStreamType(hwAsic);
    queue9.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
    queue9.weight() = utility::kCoppHighPriWeight;
    if (addEcnConfig) {
      queue9.aqms() = {};
      queue9.aqms()->push_back(utility::kGetOlympicEcnConfig());
    }
    cpuQueues.push_back(queue9);

    config.cpuQueues() = cpuQueues;
  }

 private:
  folly::Synchronized<pktReceivedCb, std::mutex> pktReceivedCallback_;
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }
};

class HwCoppQueueStuckTest : public HwCoppQosTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::twoL3IntfConfig(
        getHwSwitch(),
        masterLogicalInterfacePortIds()[0],
        masterLogicalInterfacePortIds()[1],
        getAsic()->desiredLoopbackMode());
    utility::setDefaultCpuTrafficPolicyConfig(cfg, getAsic());
    addCustomCpuQueueConfig(cfg, getAsic(), true /*addEcnConfig*/);
    return cfg;
  }
};

TYPED_TEST_SUITE(HwCoppTest, TestTypes);

TYPED_TEST(HwCoppTest, VerifyCoppPpsLowPri) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    auto kNumPktsToSend = 60000;
    auto kMinDurationInSecs = 12;
    const double kVariance = 0.30; // i.e. + or -30%

    auto beforeSecs = getCurrentTime();
    auto beforeOutPkts = this->getQueueOutPacketsWithRetry(
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
    do {
      this->sendTcpPkts(
          kNumPktsToSend,
          dstIP,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);
      afterSecs = getCurrentTime();
    } while (afterSecs - beforeSecs < kMinDurationInSecs);

    auto afterOutPkts = this->getQueueOutPacketsWithRetry(
        utility::kCoppLowPriQueueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    auto totalRecvdPkts = afterOutPkts - beforeOutPkts;
    auto duration = afterSecs - beforeSecs;
    auto currPktsPerSec = totalRecvdPkts / duration;
    uint32_t lowPriorityPps =
        utility::getCoppQueuePps(this->getAsic(), utility::kCoppLowPriQueueId);
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

TYPED_TEST(HwCoppTest, LocalDstIpBgpPortToHighPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    /*
     * XXX BCM_GPORT_LOCAL_CPU (335544320) does not work as it is different
     * from * portGport (134217728)?
     */
    // Make sure all dstip(=interfaces local ips) + BGP port packets send to
    // cpu high priority queue
    enum SRC_DST { SRC, DST };
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      for (int dir = 0; dir <= DST; dir++) {
        XLOG(DBG2) << "Send Pkt to: " << ipAddress
                   << " dir: " << (dir == DST ? " DST" : " SRC");
        this->sendPktAndVerifyCpuQueue(
            utility::getCoppHighPriQueueId(this->getAsic()),
            folly::IPAddress::createNetwork(ipAddress, -1, false).first,
            dir == SRC ? utility::kBgpPort : utility::kNonSpecialPort1,
            dir == DST ? utility::kBgpPort : utility::kNonSpecialPort1);
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, LocalDstIpNonBgpPortToMidPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      this->sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(this->getAsic()),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalMcastToMidPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      this->sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          kMcastMacAddress);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(this->getAsic()),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalMcastTxFromCpu) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    // Intent of this test is to verify that
    // link local ipv6 address is not looped back when sent from CPU
    this->sendUdpPktAndVerify(
        utility::kCoppMidPriQueueId,
        folly::IPAddressV6("ff02::1"),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        1 /* send pkt count */,
        0 /* expected rx count */);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalUcastToMidPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    // Device link local unicast address should use mid-pri queue
    {
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, this->getPlatform()->getLocalMac());

      this->sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(this->getAsic()),
              kGetQueueOutPktsRetryTimes,
              0));
    }
    // Non device link local unicast address should also use mid-pri queue
    {
      this->sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2);

      // Also high-pri queue should always be 0
      EXPECT_EQ(
          0,
          this->getQueueOutPacketsWithRetry(
              utility::getCoppHighPriQueueId(this->getAsic()),
              kGetQueueOutPktsRetryTimes,
              0));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, SlowProtocolsMacToHighPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    this->sendPktAndVerifyEthPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        facebook::fboss::ETHERTYPE::ETHERTYPE_SLOW_PROTOCOLS,
        LACPDU::kSlowProtocolsDstMac());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, EapolToHighPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    this->sendPktAndVerifyEthPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        facebook::fboss::ETHERTYPE::ETHERTYPE_EAPOL,
        folly::MacAddress("ff:ff:ff:ff:ff:ff"));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, DstIpNetworkControlDscpToHighPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
    // Non local dst ip with kNetworkControlDscp should not hit high pri queue
    // (since it won't even trap to cpu)
    this->sendPktAndVerifyCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        folly::IPAddress("2::2"),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt,
        kNetworkControlDscp,
        1, /*num pkts to send*/
        0 /*expected delta*/);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalUcastIpNetworkControlDscpToHighPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    // Device link local unicast address + kNetworkControlDscp should use
    // high-pri queue
    {
      const folly::IPAddressV6 linkLocalAddr = folly::IPAddressV6(
          folly::IPAddressV6::LINK_LOCAL, this->getPlatform()->getLocalMac());

      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          linkLocalAddr,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
    // Non device link local unicast address + kNetworkControlDscp dscp should
    // also use high-pri queue
    {
      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          kIPv6LinkLocalUcastAddress,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt,
          kNetworkControlDscp);
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ipv6LinkLocalMcastNetworkControlDscpToHighPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    const auto addresses = folly::make_array(
        kIPv6LinkLocalMcastAbsoluteAddress, kIPv6LinkLocalMcastAddress);
    for (const auto& address : addresses) {
      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          address,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          kMcastMacAddress,
          kNetworkControlDscp);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, L3MTUErrorToLowPriQ) {
  auto setup = [=]() {
    this->setup();
    this->setupEcmp();
  };
  auto verify = [=]() {
    // Make sure all packets packet with large payload (> MTU)
    // are sent to cpu low priority queue.
    // Port Max Frame size is set to 9412 and L3 MTU is set as 9000
    // Thus sending a packet sized between 9000 and 9412 to cause the trap.
    auto randomIP = folly::IPAddressV6("2::2");
    this->sendPktAndVerifyCpuQueue(
        utility::kCoppLowPriQueueId,
        randomIP,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt,
        0,
        1, /* num pkts to send */
        1, /* num pkts to excepted to be captured */
        std::vector<uint8_t>(9200, 0xff));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, ArpRequestAndReplyToHighPriQ) {
  auto setup = [=]() { this->setup(); };
  auto verify = [=]() {
    this->sendPktAndVerifyArpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        folly::IPAddressV4("1.1.1.5"),
        ARP_OPER::ARP_OPER_REQUEST);
    this->sendPktAndVerifyArpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        folly::IPAddressV4("1.1.1.5"),
        ARP_OPER::ARP_OPER_REPLY);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, NdpSolicitationToHighPriQ) {
  auto setup = [=]() { this->setup(); };
  auto verify = [=]() {
    XLOG(DBG2) << "verifying solicitation";
    this->sendPktAndVerifyNdpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        folly::IPAddressV6("1::2"), // sender of solicitation
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, NdpSolicitNeighbor) {
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
  auto setup = [=]() { this->setup(); };
  auto verify = [=]() {
    XLOG(DBG2) << "verifying solicitation";
    this->sendPktAndVerifyNdpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        folly::IPAddressV6("1::1"), // sender of solicitation
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
        false,
        false,
        1,
        1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, NdpAdvertisementToHighPriQ) {
  auto setup = [=]() { this->setup(); };
  auto verify = [=]() {
    XLOG(DBG2) << "verifying advertisement";
    this->sendPktAndVerifyNdpPacketsCpuQueue(
        utility::getCoppHighPriQueueId(this->getAsic()),
        folly::IPAddressV6("1::2"), // sender of advertisement
        ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, UnresolvedRoutesToLowPriQueue) {
  auto setup = [=]() {
    this->setup();
    utility::EcmpSetupAnyNPorts6 ecmp6(this->getProgrammedState());
    ecmp6.programRoutes(this->getRouteUpdater(), 1);
  };
  auto randomIP = folly::IPAddressV6("2::2");
  auto verify = [=]() {
    this->sendPktAndVerifyCpuQueue(
        utility::kCoppLowPriQueueId,
        randomIP,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt,
        0,
        1, /* num pkts to send */
        1 /* num pkts to excepted to be captured */);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, JumboFramesToQueues) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    std::vector<uint8_t> jumboPayload(7000, 0xff);
    for (const auto& ipAddress : this->getIpAddrsToSendPktsTo()) {
      // High pri queue
      this->sendPktAndVerifyCpuQueue(
          utility::getCoppHighPriQueueId(this->getAsic()),
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kBgpPort,
          utility::kNonSpecialPort2,
          std::nullopt, /*mac*/
          0, /* traffic class*/
          1, /* pkts to send*/
          1, /* expected delta*/
          jumboPayload);
      // Mid pri queue
      this->sendPktAndVerifyCpuQueue(
          utility::kCoppMidPriQueueId,
          folly::IPAddress::createNetwork(ipAddress, -1, false).first,
          utility::kNonSpecialPort1,
          utility::kNonSpecialPort2,
          std::nullopt, /*mac*/
          0, /* traffic class*/
          1, /* pkts to send*/
          1, /* expected delta*/
          jumboPayload);
    }
    this->sendPktAndVerifyCpuQueue(
        utility::kCoppLowPriQueueId,
        this->getInSubnetNonSwitchIP(),
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        std::nullopt, /*mac*/
        0, /* traffic class*/
        1, /* pkts to send*/
        1, /* expected delta*/
        jumboPayload);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppQueueStuckTest, CpuQueueHighRateTraffic) {
  constexpr int kTestIterations = 6;
  constexpr int kTestIterationWaitInSeconds = 5;

  auto setup = [=]() { setupEcmpDataplaneLoop(); };

  auto verify = [&]() {
    const auto ipForLowPriorityQueue = folly::IPAddress("4::1");

    // Create trap entry to punt data traffic to CPU low pri queue
    auto ipAddress = folly::CIDRNetwork{ipForLowPriorityQueue, 128};
    auto packetCapture = HwTestPacketTrapEntry(getHwSwitch(), ipAddress);

    // Create dataplane loop with lowerPriority traffic on port0
    auto baseVlan = utility::firstVlanID(initialConfig());
    createLineRateTrafficOnPort(
        masterLogicalInterfacePortIds()[0], baseVlan, ipForLowPriorityQueue);

    bool lowPriorityTrafficMissing{false};
    uint64_t previousLowPriorityPacketCount{};
    /*
     * Running the test for atleast kTestIterations. As we have
     * traffic at close to line rate being received on CPU low
     * priority queue, this is enough to validate possible queue
     * stuck condition.
     */
    for (int iter = 0; iter < kTestIterations; iter++) {
      std::this_thread::sleep_for(
          std::chrono::seconds(kTestIterationWaitInSeconds));
      // Read low priority copp queue counters
      uint64_t lowPriorityPacketCount =
          utility::getCpuQueueOutPacketsAndBytes(
              getHwSwitch(), utility::kCoppLowPriQueueId)
              .first;
      XLOG(DBG0) << "Received packet count: " << lowPriorityPacketCount;
      /*
       * Make sure that COPP queue keeps moving! As we are sending
       * close to line rate low priority packets, we dont expect it
       * to be read without an increment.  Queue not incrementing in
       * a single iteration is good enough to flag a stuck condition
       * given each iteration waits long enough.
       */
      if (lowPriorityPacketCount == previousLowPriorityPacketCount) {
        lowPriorityTrafficMissing = true;
        break;
      }
      previousLowPriorityPacketCount = lowPriorityPacketCount;
    }

    EXPECT_FALSE(lowPriorityTrafficMissing);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwCoppQosTest, HighVsLowerPriorityCpuQueueTrafficPrioritization) {
  constexpr int kReceiveRetriesInSeconds = 2;
  constexpr int kTransmitRetriesInSeconds = 5;
  constexpr int kHighPriorityPacketCount = 30000;
  constexpr int packetsPerBurst = 1000;
  std::map<folly::IPAddress, uint64_t> rxPktCountMap{};

  auto setup = [=]() { setupEcmpDataplaneLoop(); };

  auto pktReceiveHandler = [&](RxPacket* pkt) {
    auto destinationAddress = getDestinationIpIfValid(pkt);
    if (destinationAddress.has_value()) {
      rxPktCountMap[destinationAddress.value()]++;
    }
  };

  auto verify = [&]() {
    auto configIntf = folly::copy(*(this->initialConfig()).interfaces())[1];
    const auto ipForHighPriorityQueue =
        folly::IPAddress::createNetwork(configIntf.ipAddresses()[1], -1, false)
            .first;
    const auto ipForLowPriorityQueue = folly::IPAddress("4::1");

    // Register packet receive callback
    auto baseVlan = utility::firstVlanID(initialConfig());
    registerPktReceivedCallback(pktReceiveHandler);

    // Create trap entry to punt data traffic to CPU low pri queue
    auto ipAddress = folly::CIDRNetwork{ipForLowPriorityQueue, 128};
    auto packetCapture = HwTestPacketTrapEntry(getHwSwitch(), ipAddress);

    // Get initial packet count on port1 for high priority traffic
    auto initialHighPriorityPacketCount =
        getLatestPortStats(masterLogicalInterfacePortIds()[1])
            .get_outUnicastPkts_();

    // Create dataplane loop with lowerPriority traffic on port0
    createLineRateTrafficOnPort(
        masterLogicalInterfacePortIds()[0], baseVlan, ipForLowPriorityQueue);
    std::optional<VlanID> nextVlan;
    if (baseVlan) {
      nextVlan = *baseVlan + 1;
    }

    // Send a fixed number of high priority packets on port1
    sendPacketBursts(
        masterLogicalInterfacePortIds()[1],
        nextVlan,
        kHighPriorityPacketCount,
        packetsPerBurst,
        ipForHighPriorityQueue,
        utility::kNonSpecialPort1,
        utility::kBgpPort);

    auto allHighPriorityPacketsSent = [&](const auto& newStats) {
      auto portStatsIter = newStats.find(masterLogicalInterfacePortIds()[1]);
      auto outCount = (portStatsIter == newStats.end())
          ? 0
          : portStatsIter->second.get_outUnicastPkts_();
      return outCount ==
          (kHighPriorityPacketCount + initialHighPriorityPacketCount);
    };

    // Make sure all the high priority packets have been sent
    getHwSwitchEnsemble()->waitPortStatsCondition(
        allHighPriorityPacketsSent,
        kTransmitRetriesInSeconds,
        std::chrono::milliseconds(std::chrono::seconds(1)));
    // Check high priority queue stats to see if all packets are received
    auto highPriorityCoppQueueStats = utility::getQueueOutPacketsWithRetry(
        getHwSwitch(),
        utility::getCoppHighPriQueueId(getAsic()),
        kReceiveRetriesInSeconds,
        kHighPriorityPacketCount);

    // Unregister packet received callback
    unRegisterPktReceivedCallback();

    XLOG(DBG0) << "Received packet count  -> HighPriority:"
               << rxPktCountMap[ipForHighPriorityQueue]
               << ", LowerPriority:" << rxPktCountMap[ipForLowPriorityQueue];

    uint64_t lowerPriorityCoppQueueStats =
        utility::getCpuQueueOutPacketsAndBytes(
            getHwSwitch(), utility::kCoppLowPriQueueId)
            .first;
    /*
     * Stats on lower priority queue will not be same as above
     * as traffic continues to be punted, printing for reference
     */
    XLOG(DBG0) << "COPP queue packet count-> HighPriority:"
               << highPriorityCoppQueueStats
               << ", LowerPriority:" << lowerPriorityCoppQueueStats;

    // Get the drop stats on the high and lower priority CPU queues
    auto highPriorityCoppQueueDiscardStats =
        utility::getCpuQueueOutDiscardPacketsAndBytes(
            getHwSwitch(), utility::getCoppHighPriQueueId(getAsic()))
            .first;
    auto lowerPriorityCoppQueueDiscardStats =
        utility::getCpuQueueOutDiscardPacketsAndBytes(
            getHwSwitch(), utility::kCoppLowPriQueueId)
            .first;
    XLOG(DBG0) << "COPP queue drop count  -> HighPriority:"
               << highPriorityCoppQueueDiscardStats
               << ", LowerPriority:" << lowerPriorityCoppQueueDiscardStats;

    // Test fails if there is a drop in the high priority queue
    EXPECT_EQ(highPriorityCoppQueueDiscardStats, 0);

    /*
     * Test passes if all the high priority packets sent are received at
     * application layer, this will verify no drops after dequeue from
     * the COPP queue.
     */
    EXPECT_EQ(kHighPriorityPacketCount, rxPktCountMap[ipForHighPriorityQueue]);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, LldpProtocolToMidPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    this->sendPktAndVerifyLldpPacketsCpuQueue(utility::kCoppMidPriQueueId);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, Ttl1PacketToLowPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    auto randomIP = folly::IPAddressV6("2::2");
    this->sendUdpPktAndVerify(
        utility::kCoppLowPriQueueId,
        randomIP,
        utility::kNonSpecialPort1,
        utility::kNonSpecialPort2,
        1 /* send pkt count */,
        1 /* expected rx count */,
        1 /* TTL */,
        true /* send out of port */);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, DhcpPacketToMidPriQ) {
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
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
            1 /* send pkt count */,
            1 /* expected rx count */,
            255 /* TTL */,
            true /* send out of port */);
        XLOG(DBG0) << "Sending packet with src port " << l4SrcPort
                   << " dst port " << l4DstPort << " IP: " << randomSrcIP[i];
      }
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, DHCPv6SolicitToMidPriQ) {
  auto setup = [=]() { this->setup(); };
  auto verify = [=]() {
    XLOG(DBG2) << "verifying DHCPv6 solicitation with TTL 1";
    this->sendPktAndVerifyDHCPv6PacketsCpuQueue(
        utility::kCoppMidPriQueueId, DHCPv6Type::DHCPv6_SOLICIT);
    XLOG(DBG2) << "verifying DHCPv6 solicitation with TTL 128";
    this->sendPktAndVerifyDHCPv6PacketsCpuQueue(
        utility::kCoppMidPriQueueId, DHCPv6Type::DHCPv6_SOLICIT, 128);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwCoppTest, DHCPv6AdvertiseToMidPriQ) {
  auto setup = [=]() { this->setup(); };
  auto verify = [=]() {
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
