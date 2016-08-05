/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <gtest/gtest.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/gen-cpp/switch_config_types.h"

#include <future>
#include <netinet/icmp6.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

using namespace facebook::fboss;
using facebook::network::toIPAddress;
using facebook::network::toBinaryAddress;
using folly::IPAddressV6;
using folly::MacAddress;
using facebook::network::thrift::BinaryAddress;
using folly::io::Cursor;
using folly::StringPiece;
using std::unique_ptr;
using std::shared_ptr;
using std::chrono::seconds;

using ::testing::_;

namespace {

const MacAddress kPlatformMac("02:01:02:03:04:05");

cfg::SwitchConfig createSwitchConfig(seconds raInterval, seconds ndpTimeout) {
  // Create a thrift config to use
  cfg::SwitchConfig config;
  config.vlans.resize(1);
  config.vlans[0].name = "PrimaryVlan";
  config.vlans[0].id = 5;
  config.vlans[0].routable = true;
  config.vlans[0].intfID = 1234;

  config.vlanPorts.resize(10);
  config.ports.resize(10);
  for (int n = 0; n < 10; ++n) {
    config.ports[n].logicalID = n + 1;
    config.ports[n].state = cfg::PortState::UP;
    config.ports[n].minFrameSize = 64;
    config.ports[n].maxFrameSize = 9000;
    config.ports[n].routable = true;
    config.ports[n].ingressVlan = 5;

    config.vlanPorts[n].vlanID = 5;
    config.vlanPorts[n].logicalPort = n + 1;
    config.vlanPorts[n].spanningTreeState = cfg::SpanningTreeState::FORWARDING;
    config.vlanPorts[n].emitTags = 0;
  }

  config.interfaces.resize(1);
  config.interfaces[0].intfID = 1234;
  config.interfaces[0].vlanID = 5;
  config.interfaces[0].name = "PrimaryInterface";
  config.interfaces[0].mtu = 9000;
  config.interfaces[0].__isset.mtu = true;
  config.interfaces[0].ipAddresses.resize(5);
  config.interfaces[0].ipAddresses[0] = "10.164.4.10/24";
  config.interfaces[0].ipAddresses[1] = "10.164.4.1/24";
  config.interfaces[0].ipAddresses[2] = "10.164.4.2/24";
  config.interfaces[0].ipAddresses[3] = "2401:db00:2110:3004::/64";
  config.interfaces[0].ipAddresses[4] = "2401:db00:2110:3004::000a/64";
  config.interfaces[0].__isset.ndp = true;
  config.interfaces[0].ndp.routerAdvertisementSeconds = raInterval.count();

  if (ndpTimeout.count() > 0) {
    config.arpTimeoutSeconds = ndpTimeout.count();
  }

  return config;
}

unique_ptr<SwSwitch> setupSwitch(seconds raInterval,
                                 seconds ndpInterval) {
  auto config = createSwitchConfig(raInterval, ndpInterval);

  config.maxNeighborProbes = 1;
  config.staleEntryInterval = 1;

  auto sw = createMockSw(&config, kPlatformMac);
  sw->initialConfigApplied(std::chrono::steady_clock::now());
  return sw;
}

shared_ptr<SwitchState> addMockRouteTable(shared_ptr<SwitchState> state) {
  RouteNextHops nexthops;
  nexthops.emplace(IPAddressV6("2401:db00:2110:3004::1")); // resolved by intf 1
  nexthops.emplace(IPAddressV6("2401:db00:2110:3004::2")); // resolved by intf 1
  nexthops.emplace(IPAddressV6("5555:db00:2110:3004::1")); // un-resolvable

  RouteUpdater updater(state->getRouteTables());
  updater.addRoute(RouterID(0), IPAddressV6("1111:1111:1:1::1"), 64, nexthops);

  auto newRt = updater.updateDone();
  auto newState = state->clone();
  newState->resetRouteTables(newRt);
  return newState;
}

unique_ptr<SwSwitch> setupSwitch() {
  return setupSwitch(seconds(0), seconds(0));
}

unique_ptr<SwSwitch> setupSwitchWithNdpTimeout(seconds ndpTimeout) {
  return setupSwitch(seconds(0), ndpTimeout);
}

typedef std::function<void(Cursor* cursor, uint32_t length)> PayloadCheckFn;

TxMatchFn checkIMCPv6Pkt(MacAddress srcMac, IPAddressV6 srcIP,
                         MacAddress dstMac, IPAddressV6 dstIP,
                         VlanID vlan, ICMPv6Type type,
                         const PayloadCheckFn& checkPayload) {
  return [=](const TxPacket* pkt) {
    Cursor c(pkt->buf());
    auto parsedDstMac = PktUtil::readMac(&c);
    checkField(dstMac, parsedDstMac, "dst mac");
    auto parsedSrcMac = PktUtil::readMac(&c);
    checkField(srcMac, parsedSrcMac, "src mac");
    auto vlanType = c.readBE<uint16_t>();
    checkField(ETHERTYPE_VLAN, vlanType, "VLAN ethertype");
    auto vlanTag = c.readBE<uint16_t>();
    checkField(static_cast<uint16_t>(vlan), vlanTag, "VLAN tag");
    auto ethertype = c.readBE<uint16_t>();
    checkField(ETHERTYPE_IPV6, ethertype, "ethertype");
    IPv6Hdr ipv6(c);
    checkField(IP_PROTO_IPV6_ICMP, ipv6.nextHeader, "IPv6 protocol");
    checkField(srcIP, ipv6.srcAddr, "src IP");
    checkField(dstIP, ipv6.dstAddr, "dst IP");

    Cursor ipv6PayloadStart(c);
    ICMPHdr icmp6(c);
    checkField(icmp6.computeChecksum(ipv6, c), icmp6.csum, "ICMPv6 checksum");
    checkField(type, icmp6.type, "ICMPv6 type");
    checkField(0, icmp6.code, "ICMPv6 code");

    checkPayload(&c, ipv6.payloadLength - ICMPHdr::SIZE);

    if (ipv6.payloadLength != (c - ipv6PayloadStart)) {
      throw FbossError("IPv6 payload length mismatch: header says ",
                       ipv6.payloadLength, " but we used ",
                       c - ipv6PayloadStart);
    }

    // This is a match
    return;
  };
}

TxMatchFn checkNeighborAdvert(MacAddress srcMac, IPAddressV6 srcIP,
                              MacAddress dstMac, IPAddressV6 dstIP,
                              VlanID vlan, uint8_t flags) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    auto parsedFlags = cursor->read<uint8_t>();
    checkField(flags, parsedFlags, "NA flags");
    checkField(0, cursor->read<uint8_t>(), "reserved1");
    checkField(0, cursor->read<uint8_t>(), "reserved2");
    checkField(0, cursor->read<uint8_t>(), "reserved3");
    auto targetIP = PktUtil::readIPv6(cursor);
    checkField(srcIP, targetIP, "target IP");

    auto optionType = cursor->read<uint8_t>();
    checkField(2, optionType, "target MAC option type");
    auto optionLength = cursor->read<uint8_t>();
    checkField(1, optionLength, "target MAC option length");
    auto targetMac = PktUtil::readMac(cursor);
    checkField(srcMac, targetMac, "target MAC");

    // This is a match
    return;
  };
  return checkIMCPv6Pkt(srcMac, srcIP, dstMac, dstIP, vlan,
                        ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
                        checkPayload);
}

TxMatchFn checkNeighborSolicitation(MacAddress srcMac, IPAddressV6 srcIP,
                                    MacAddress dstMac, IPAddressV6 dstIP,
                                    IPAddressV6 targetIP, VlanID vlan) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    auto reserved = cursor->read<uint32_t>();
    checkField(0, reserved, "NS reserved field");
    auto parsedTargetIP = PktUtil::readIPv6(cursor);
    checkField(targetIP, parsedTargetIP, "target IP");

    auto optionType = cursor->read<uint8_t>();
    checkField(1, optionType, "source MAC option type");
    auto optionLength = cursor->read<uint8_t>();
    checkField(1, optionLength, "source MAC option length");
    auto srcMacOption = PktUtil::readMac(cursor);
    checkField(srcMac, srcMacOption, "source MAC option value");
  };
  return checkIMCPv6Pkt(srcMac, srcIP, dstMac, dstIP, vlan,
                        ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
                        checkPayload);
}

typedef std::vector<std::pair<IPAddressV6, uint8_t>> PrefixVector;
TxMatchFn checkRouterAdvert(MacAddress srcMac, IPAddressV6 srcIP,
                            MacAddress dstMac, IPAddressV6 dstIP,
                            VlanID vlan, const cfg::NdpConfig& ndp,
                            uint32_t mtu,
                            PrefixVector expectedPrefixes) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    Cursor start(*cursor);
    Cursor end = start + length;
    auto parsedHopLimit = cursor->read<uint8_t>();
    checkField(ndp.curHopLimit, parsedHopLimit, "cur hop limit");
    auto parsedFlags = cursor->read<uint8_t>();
    checkField(0, parsedFlags, "NDP RA flags");
    auto parsedLifetime = cursor->readBE<uint16_t>();
    checkField(ndp.routerLifetime, parsedLifetime, "router lifetime");
    auto parsedReachableTime = cursor->readBE<uint32_t>();
    checkField(0, parsedReachableTime, "reachable time");
    auto parsedRetransTimer = cursor->readBE<uint32_t>();
    checkField(0, parsedRetransTimer, "retransmit timer");

    bool srcMacSeen = false;
    bool mtuSeen = false;
    PrefixVector prefixes;
    while (*cursor != end) {
      auto optionType = cursor->read<uint8_t>();
      auto optionLength = cursor->read<uint8_t>();
      // TODO: define constants for the option types
      switch (optionType) {
        case 1: {
          // src link-layer address
          checkField(1, optionLength, "src MAC option length");
          if (srcMacSeen) {
            throw FbossError("duplicate src MAC option found");
          }
          srcMacSeen = true;
          auto mac = PktUtil::readMac(cursor);
          checkField(srcMac, mac, "src MAC option");
          break;
        }
        case 3: {
          // prefix
          checkField(4, optionLength, "prefix option length");
          auto prefixLength = cursor->read<uint8_t>();
          auto prefixFlags = cursor->read<uint8_t>();
          checkField(0xc0, prefixFlags, "prefix flags");
          auto prefixValidLifetime = cursor->readBE<uint32_t>();
          checkField(ndp.prefixValidLifetimeSeconds, prefixValidLifetime,
                     "prefix valid lifetime");
          auto prefixPreferredLifetime = cursor->readBE<uint32_t>();
          checkField(ndp.prefixPreferredLifetimeSeconds,
                     prefixPreferredLifetime, "prefix preferred lifetime");
          auto reserved2 = cursor->readBE<uint32_t>();
          checkField(0, reserved2, "prefix option reserved2");
          auto prefix = PktUtil::readIPv6(cursor);
          prefixes.emplace_back(prefix, prefixLength);
          break;
        }
        case 5: {
          // MTU
          checkField(1, optionLength, "MTU option length");
          if (mtuSeen) {
            throw FbossError("duplicate MTU option found");
          }
          mtuSeen = true;
          auto reserved = cursor->readBE<uint16_t>();
          checkField(0, reserved, "MTU option reserved bytes");
          auto parsedMTU = cursor->readBE<uint32_t>();
          checkField(mtu, parsedMTU, "MTU option value");
          break;
        }
        default:
          // unexpected option type
          throw FbossError("unexpected NDP option type ", optionType,
                           " at payload offset ", (*cursor - start) - 2,
                           "\n", PktUtil::hexDump(start));
      }
    }

    if (!srcMacSeen) {
      throw FbossError("no src MAC option found");
    }
    if (!mtuSeen) {
      throw FbossError("no MTU option found");
    }
    if (prefixes != expectedPrefixes) {
      // TODO: Print out useful info about the differences
      throw FbossError("mismatching advertised prefixes");
    }
  };
  return checkIMCPv6Pkt(srcMac, srcIP, dstMac, dstIP, vlan,
                        ICMPV6_TYPE_NDP_ROUTER_ADVERTISEMENT, checkPayload);
}

void sendNeighborAdvertisement(SwSwitch* sw, StringPiece ipStr,
                               StringPiece macStr, int port, int vlanID) {
  IPAddressV6 srcIP(ipStr);
  MacAddress srcMac(macStr);
  VlanID vlan(vlanID);

  // The destination MAC and IP are specific to the mock switch we setup
  MacAddress dstMac(kPlatformMac);
  IPAddressV6 dstIP("2401:db00:2110:3004::a");
  size_t plen = 20;

  IPv6Hdr ipv6(srcIP, dstIP);
  ipv6.trafficClass = 0xe0;
  ipv6.payloadLength = ICMPHdr::SIZE + plen;
  ipv6.nextHeader = IP_PROTO_IPV6_ICMP;
  ipv6.hopLimit = 255;

  size_t totalLen = EthHdr::SIZE + IPv6Hdr::SIZE + ipv6.payloadLength;
  auto buf = folly::IOBuf::create(totalLen);
  buf->append(totalLen);
  folly::io::RWPrivateCursor cursor(buf.get());

  auto bodyFn = [&] (folly::io::RWPrivateCursor *c) {
    c->write<uint32_t>(ND_NA_FLAG_OVERRIDE | ND_NA_FLAG_SOLICITED);
    c->push(srcIP.bytes(), IPAddressV6::byteCount());
  };

  ICMPHdr icmp6(ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT, 0, 0);
  icmp6.serializeFullPacket(&cursor, dstMac, srcMac, vlan, ipv6, plen, bodyFn);

  auto pkt = folly::make_unique<MockRxPacket>(std::move(buf));
  pkt->padToLength(totalLen);
  pkt->setSrcPort(PortID(port));
  pkt->setSrcVlan(vlan);

  // Send the packet to the switch
  sw->packetReceived(std::move(pkt));
}

} // unnamed namespace

TEST(NdpTest, UnsolicitedRequest) {
  auto sw = setupSwitch();

  // Create an neighbor solicitation request
  auto pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "33 33 ff 00 00 0a  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 58 (ICMPv6), Hop Limit (255)
      "3a ff"
      // src addr (::0)
      "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
      // dst addr (ff02::1:ff00:000a)
      "ff 02 00 00 00 00 00 00 00 00 00 01 ff 00 00 0a"
      // type: neighbor solicitation
      "87"
      // code
      "00"
      // checksum
      "2a 7e"
      // reserved
      "00 00 00 00"
      // target address (2401:db00:2110:3004::a)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 00 00 0a");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(5));

  // Cache the current stats
  CounterCache counters(sw.get());

  // We should get a neighbor advertisement back
  EXPECT_PKT(sw, "neighbor advertisement",
             checkNeighborAdvert(kPlatformMac,
                                 IPAddressV6("2401:db00:2110:3004::a"),
                                 MacAddress("02:05:73:f9:46:fc"),
                                 IPAddressV6("ff01::1"),
                                 VlanID(5), 0xa0));

  // Send the packet to the SwSwitch
  sw->packetReceived(std::move(pkt));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 1);
}

TEST(NdpTest, TriggerSolicitation) {
  auto sw = setupSwitch();
  sw->updateStateBlocking("add test route table", addMockRouteTable);

  // Create a packet to a node in the attached IPv6 subnet
  auto pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (2401:db00:2110:3004::1:0)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 01 00 00"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(5));

  // Cache the current stats
  CounterCache counters(sw.get());

  // We should get a neighbor solicitation back
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:01:00:00"),
        IPAddressV6("ff02::1:ff01:0"),
        IPAddressV6("2401:db00:2110:3004::1:0"),
        VlanID(5)));

  // Send the packet to the SwSwitch
  sw->packetReceived(std::move(pkt));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);

  // Create a packet to a node not in attached subnet, but in route table
  pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (1111:1111:1:1:2:3:4:5)
      "11 11 11 11 00 01 00 01 00 02 00 03 00 04 00 05"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(5));

  // We should get a neighbor solicitation back
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:01"),
        IPAddressV6("ff02::1:ff00:1"),
        IPAddressV6("2401:db00:2110:3004::1"),
        VlanID(5)));

  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:02"),
        IPAddressV6("ff02::1:ff00:2"),
        IPAddressV6("2401:db00:2110:3004::2"),
        VlanID(5)));

  // Send the packet to the SwSwitch
  sw->packetReceived(std::move(pkt));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
}

TEST(NdpTest, RouterAdvertisement) {
  seconds raInterval(1);
  auto config = createSwitchConfig(raInterval, seconds(0));
  // Add an interface with a /128 mask, to make sure it isn't included
  // in the generated RA packets.
  config.interfaces[0].ipAddresses.push_back("2401:db00:2000:1234:1::/128");
  auto sw = createMockSw(&config, kPlatformMac);
  sw->initialConfigApplied(std::chrono::steady_clock::now());

  auto state = sw->getState();
  auto intfConfig = state->getInterfaces()->getInterface(InterfaceID(1234));
  PrefixVector expectedPrefixes{
    { IPAddressV6("2401:db00:2110:3004::"), 64 },
  };
  EXPECT_PKT(sw, "router advertisement",
             checkRouterAdvert(kPlatformMac,
                               IPAddressV6("fe80::1:02ff:fe03:0405"),
                               MacAddress("33:33:00:00:00:01"),
                               IPAddressV6("ff02::1"),
                               VlanID(5), intfConfig->getNdpConfig(),
                               9000, expectedPrefixes));

  // Send the router solicitation packet
  EXPECT_PKT(sw, "router advertisement",
             checkRouterAdvert(kPlatformMac,
                               IPAddressV6("fe80::1:02ff:fe03:0405"),
                               MacAddress("02:05:73:f9:46:fc"),
                               IPAddressV6("2401:db00:2110:1234::1:0"),
                               VlanID(5), intfConfig->getNdpConfig(),
                               9000, expectedPrefixes));

  auto pkt = MockRxPacket::fromHex(
    // dst mac, src mac
    "33 33 00 00 00 02  02 05 73 f9 46 fc"
    // 802.1q, VLAN 5
    "81 00 00 05"
    // IPv6
    "86 dd"
    // Version 6, traffic class, flow label
    "6e 00 00 00"
    // Payload length: 8
    "00 08"
    // Next Header: 58 (ICMPv6), Hop Limit (255)
    "3a ff"
    // src addr (2401:db00:2110:1234::1:0)
    "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
    // dst addr (ff02::2)
    "ff 02 00 00 00 00 00 00 00 00 00 00 00 00 00 02"
    // type: router solicitation
    "85"
    // code
    "00"
    // checksum
    "49 71"
    // reserved
    "00 00 00 00"
  );
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(5));
  sw->packetReceived(std::move(pkt));


  // Now send the packet with specified source MAC address as ICMPv6 option
  // which differs from MAC address in ethernet header. The switch should use
  // the MAC address in options field.

  EXPECT_PKT(sw, "router advertisement",
             checkRouterAdvert(kPlatformMac,
                               IPAddressV6("fe80::1:02ff:fe03:0405"),
                               MacAddress("02:ab:73:f9:46:fc"),
                               IPAddressV6("2401:db00:2110:1234::1:0"),
                               VlanID(5), intfConfig->getNdpConfig(),
                               9000, expectedPrefixes));

  auto pkt2 = MockRxPacket::fromHex(
    // dst mac, src mac
    "33 33 00 00 00 02  02 05 73 f9 46 fc"
    // 802.1q, VLAN 5
    "81 00 00 05"
    // IPv6
    "86 dd"
    // Version 6, traffic class, flow label
    "6e 00 00 00"
    // Payload length: 16
    "00 10"
    // Next Header: 58 (ICMPv6), Hop Limit (255)
    "3a ff"
    // src addr (2401:db00:2110:1234::1:0)
    "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
    // dst addr (ff02::2)
    "ff 02 00 00 00 00 00 00 00 00 00 00 00 00 00 02"
    // type: router solicitation
    "85"
    // code
    "00"
    // checksum
    "8a c7"
    // reserved
    "00 00 00 00"
    // option type: Source Link-Layer Address, length
    "01 01"
    // source mac
    "02 ab 73 f9 46 fc"
  );
  pkt2->setSrcPort(PortID(1));
  pkt2->setSrcVlan(VlanID(5));
  sw->packetReceived(std::move(pkt2));

  // The RA packet will be sent in the background even thread after the RA
  // interval.  Schedule a timeout to wake us up after the interval has
  // expired.  Using the background EventBase to run the timeout ensures that
  // it will always run after the RA timeout has fired.
  std::promise<bool> done;
  auto* evb = sw->getBackgroundEVB();
  evb->runInEventBaseThread([&]() {
      evb->tryRunAfterDelay([&]() {
        done.set_value(true);
      }, 1010);
    });
  done.get_future().wait();
  // We send RA packets just before switch controller shutdown,
  // so expect RA packet.
  EXPECT_PKT(sw, "router advertisement",
             checkRouterAdvert(kPlatformMac,
                               IPAddressV6("fe80::1:02ff:fe03:0405"),
                               MacAddress("33:33:00:00:00:01"),
                               IPAddressV6("ff02::1"),
                               VlanID(5), intfConfig->getNdpConfig(),
                               9000, expectedPrefixes));
}

TEST(NdpTest, FlushEntry) {
  auto sw = setupSwitch();
  ThriftHandler thriftHandler(sw.get());
  std::vector<NdpEntryThrift> ndpTable;

  // Helper for checking entries in NDP table
  auto checkEntry = [&](int idx, StringPiece ip, StringPiece mac, int port) {
    SCOPED_TRACE(folly::to<std::string>("index ", idx));
    EXPECT_EQ(ndpTable[idx].vlanID, 5);
    EXPECT_EQ(ndpTable[idx].vlanName, "PrimaryVlan");
    EXPECT_EQ(toIPAddress(ndpTable[idx].ip).str(), ip);
    EXPECT_EQ(ndpTable[idx].mac, mac);
    EXPECT_EQ(ndpTable[idx].port, port);
  };

  // Send two unsolicited neighbor advertisements, state should update at
  // least once
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sendNeighborAdvertisement(sw.get(), "2401:db00:2110:3004::b",
                            "02:05:73:f9:46:fb", 1, 5);
  sendNeighborAdvertisement(sw.get(), "2401:db00:2110:3004::c",
                            "02:05:73:f9:46:fc", 1, 5);
  waitForStateUpdates(sw.get());

  // Check there are 2 entries
  thriftHandler.getNdpTable(ndpTable);
  ASSERT_EQ(ndpTable.size(), 2);
  // Sort the results so we can check the exact ordering here.
  std::sort(
    ndpTable.begin(),
    ndpTable.end(),
    [](const NdpEntryThrift& a, const NdpEntryThrift& b) {
      return a.mac < b.mac;
    }
  );
  checkEntry(0, "2401:db00:2110:3004::b", "02:05:73:f9:46:fb", 1);
  checkEntry(1, "2401:db00:2110:3004::c", "02:05:73:f9:46:fc", 1);

  // Flush 2401:db00:2110:3004::b
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  auto binAddr = toBinaryAddress(
    IPAddressV6("2401:db00:2110:3004::b"));
  auto numFlushed = thriftHandler.flushNeighborEntry(
    folly::make_unique<BinaryAddress>(binAddr), 5);
  EXPECT_EQ(numFlushed, 1);

  // Only one entry now
  ndpTable.clear();
  thriftHandler.getNdpTable(ndpTable);
  ASSERT_EQ(ndpTable.size(), 1);
  checkEntry(0, "2401:db00:2110:3004::c", "02:05:73:f9:46:fc", 1);

  // Try flushing 2401:db00:2110:3004::b again (should be a no-op)
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  binAddr = toBinaryAddress(
    IPAddressV6("2401:db00:2110:3004::b"));
  numFlushed = thriftHandler.flushNeighborEntry(
    folly::make_unique<BinaryAddress>(binAddr), 5);
  EXPECT_EQ(numFlushed, 0);

  // Still one entry
  ndpTable.clear();
  thriftHandler.getNdpTable(ndpTable);
  ASSERT_EQ(ndpTable.size(), 1);
  checkEntry(0, "2401:db00:2110:3004::c", "02:05:73:f9:46:fc", 1);

  // Now flush 2401:db00:2110:3004::c, but using special Vlan0
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  binAddr = toBinaryAddress(
    IPAddressV6("2401:db00:2110:3004::c"));
  numFlushed = thriftHandler.flushNeighborEntry(
    folly::make_unique<BinaryAddress>(binAddr), 0);
  EXPECT_EQ(numFlushed, 1);

  // Table should be empty now
  ndpTable.clear();
  thriftHandler.getNdpTable(ndpTable);
  ASSERT_EQ(ndpTable.size(), 0);
}

TEST(NdpTest, PendingNdp) {
  auto sw = setupSwitch();

  auto vlanID = VlanID(5);

   // Create a packet to a node in the attached IPv6 subnet
  auto pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (2401:db00:2110:3004::1:0)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 01 00 00"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Cache the current stats
  CounterCache counters(sw.get());

  // We should get a neighbor solicitation back and the state should change once
  // to add the pending entry
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:01:00:00"),
        IPAddressV6("ff02::1:ff01:0"),
        IPAddressV6("2401:db00:2110:3004::1:0"),
        vlanID));

  // Send the packet to the SwSwitch
  sw->packetReceived(pkt->clone());

  // Should see a pending entry now
  waitForStateUpdates(sw.get());
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  // Receiving this duplicate packet should NOT trigger an ndp solicitation out,
  // and no state update for now.
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);

  sw->packetReceived(pkt->clone());

  // Should still see a pending entry now
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  // Receive an ndp advertisement for our pending entry
  sendNeighborAdvertisement(sw.get(), "2401:db00:2110:3004::1:0",
                            "02:10:20:30:40:22", 1, vlanID);

  // The entry should now be valid instead of pending
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);

  // Verify that we don't ever overwrite a valid entry with a pending one.
  // Receive the same packet again, no state update and the entry should still
  // be valid
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);

  sw->packetReceived(pkt->clone());
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);
};


TEST(NdpTest, PendingNdpCleanup) {
  seconds ndpTimeout(1);
  auto sw = setupSwitchWithNdpTimeout(ndpTimeout);
  sw->updateStateBlocking("add test route table", addMockRouteTable);

  auto vlanID = VlanID(5);

   // Create a packet to a node in the attached IPv6 subnet
  auto pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (2401:db00:2110:3004::1:0)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 01 00 00"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Cache the current stats
  CounterCache counters(sw.get());

  // We should get a neighbor solicitation back and the state should change once
  // to add the pending entry
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:01:00:00"),
        IPAddressV6("ff02::1:ff01:0"),
        IPAddressV6("2401:db00:2110:3004::1:0"),
        vlanID));

  // Send the packet to the SwSwitch
  sw->packetReceived(pkt->clone());

  // Should see a pending entry now
  waitForStateUpdates(sw.get());
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  // Create a second packet to a node not in attached subnet, but in route table
  pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (1111:1111:1:1:2:3:4:5)
      "11 11 11 11 00 01 00 01 00 02 00 03 00 04 00 05"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(5));

  // We should send two more neighbor solicitations
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:01"),
        IPAddressV6("ff02::1:ff00:1"),
        IPAddressV6("2401:db00:2110:3004::1"),
        VlanID(5)));

  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:02"),
        IPAddressV6("ff02::1:ff00:2"),
        IPAddressV6("2401:db00:2110:3004::2"),
        VlanID(5)));

  // Send the packet to the SwSwitch
  sw->packetReceived(std::move(pkt));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);

  // Should see two more pending entries now
  waitForStateUpdates(sw.get());
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(IPAddressV6("2401:db00:2110:3004::1"));
  auto entry2 = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(IPAddressV6("2401:db00:2110:3004::2"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), true);

 // Wait for pending entries to expire
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  std::promise<bool> done;
  auto* evb = sw->getBackgroundEVB();
  evb->runInEventBaseThread([&]() {
      evb->tryRunAfterDelay([&]() {
        done.set_value(true);
      }, 1050);
    });
  done.get_future().wait();

  // Entries should be removed
  auto ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  entry = ndpTable->getEntryIf(IPAddressV6("2401:db00:2110:3004::1:0"));
  entry2 = ndpTable->getEntryIf(IPAddressV6("2401:db00:2110:3004::1"));
  auto entry3 = ndpTable->getEntryIf(IPAddressV6("2401:db00:2110:3004::2"));
  EXPECT_EQ(entry, nullptr);
  EXPECT_EQ(entry2, nullptr);
  EXPECT_EQ(entry3, nullptr);
  EXPECT_NE(sw, nullptr);
};

TEST(NdpTest, NdpExpiration) {
  seconds ndpTimeout(1);
  auto sw = setupSwitchWithNdpTimeout(ndpTimeout);
  sw->updateStateBlocking("add test route table", addMockRouteTable);

  auto vlanID = VlanID(5);

  auto targetIP = IPAddressV6("2401:db00:2110:3004::1:0");
  auto targetIP2 = IPAddressV6("2401:db00:2110:3004::1");
  auto targetIP3 = IPAddressV6("2401:db00:2110:3004::2");

   // Create a packet to a node in the attached IPv6 subnet
  auto pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (2401:db00:2110:3004::1:0)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 01 00 00"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Cache the current stats
  CounterCache counters(sw.get());

  // We should get a neighbor solicitation back and the state should change once
  // to add the pending entry
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:01:00:00"),
        IPAddressV6("ff02::1:ff01:0"),
        targetIP,
        vlanID));

  // Send the packet to the SwSwitch
  sw->packetReceived(pkt->clone());

  // Should see a pending entry now
  waitForStateUpdates(sw.get());
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(targetIP);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  // Create a second packet to a node not in attached subnet, but in route table
  pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (1111:1111:1:1:2:3:4:5)
      "11 11 11 11 00 01 00 01 00 02 00 03 00 04 00 05"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(5));

  // We should send two more neighbor solicitations
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:01"),
        IPAddressV6("ff02::1:ff00:1"),
        targetIP2,
        VlanID(5)));

  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:02"),
        IPAddressV6("ff02::1:ff00:2"),
        targetIP3,
        VlanID(5)));

  // Send the packet to the SwSwitch
  sw->packetReceived(std::move(pkt));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);

  // Should see two more pending entries now
  waitForStateUpdates(sw.get());

  auto ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  auto entry2 = ndpTable->getEntryIf(targetIP2);
  auto entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), true);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), true);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  // Receive ndp advertisements for our pending entries
  sendNeighborAdvertisement(sw.get(), targetIP.str(),
                            "02:10:20:30:40:22", 1, vlanID);
  sendNeighborAdvertisement(sw.get(), targetIP2.str(),
                            "02:10:20:30:40:23", 1, vlanID);
  sendNeighborAdvertisement(sw.get(), targetIP3.str(),
                            "02:10:20:30:40:24", 1, vlanID);

  // Have getAndClearNeighborHit return false for the first entry,
  // but true for the others. This should result in the first
  // entry not being expired, while the others should get expired.
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP)))
    .WillRepeatedly(testing::Return(false));
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP2)))
    .WillRepeatedly(testing::Return(true));
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP3)))
    .WillRepeatedly(testing::Return(true));

  // The entries should now be valid instead of pending
  waitForStateUpdates(sw.get());
  ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  entry = ndpTable->getEntryIf(targetIP);
  entry2 = ndpTable->getEntryIf(targetIP2);
  entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), false);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), false);

  // We should send two more neighbor solicitations for entry 2 & 3
  // before we expire them
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:01"),
        IPAddressV6("ff02::1:ff00:1"),
        targetIP2,
        VlanID(5)));

  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:02"),
        IPAddressV6("ff02::1:ff00:2"),
        targetIP3,
        VlanID(5)));

 // Wait for the second and third entries to expire.
 // We wait 2.5 seconds(plus change):
 // Up to 1.5 seconds for lifetime.
 // 1 more second for probe
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  std::promise<bool> done;
  auto* evb = sw->getBackgroundEVB();
  evb->runInEventBaseThread([&]() {
      evb->tryRunAfterDelay([&]() {
        done.set_value(true);
      }, 2550);
    });
  done.get_future().wait();

  // The first entry should not be expired, but the others should be
  ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  entry = ndpTable->getEntryIf(targetIP);
  entry2 = ndpTable->getEntryIf(targetIP2);
  entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);
  EXPECT_EQ(entry2, nullptr);
  EXPECT_EQ(entry3, nullptr);

  // Now return true for the getAndClearNeighborHit calls on the first entry
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP)))
    .WillRepeatedly(testing::Return(true));

  // We should see one more solicitation for entry 1 before we expire it
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:01:00:00"),
        IPAddressV6("ff02::1:ff01:0"),
        targetIP,
        vlanID));

 // Wait for the first entry to expire
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  std::promise<bool> done2;
  evb->runInEventBaseThread([&]() {
      evb->tryRunAfterDelay([&]() {
        done2.set_value(true);
      }, 2050);
    });
  done2.get_future().wait();

  // First entry should now be expired
  entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(targetIP);
  EXPECT_EQ(entry, nullptr);
}


TEST(NdpTest, PortFlapRecover) {
  seconds ndpTimeout(1);
  auto sw = setupSwitchWithNdpTimeout(ndpTimeout);
  sw->updateStateBlocking("add test route table", addMockRouteTable);

  // We only have special port down handling after the fib is
  // synced, so we set that here.
  sw->fibSynced();

  auto vlanID = VlanID(5);

  auto targetIP = IPAddressV6("2401:db00:2110:3004::1:0");
  auto targetIP2 = IPAddressV6("2401:db00:2110:3004::1");
  auto targetIP3 = IPAddressV6("2401:db00:2110:3004::2");

   // Create a packet to a node in the attached IPv6 subnet
  auto pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (2401:db00:2110:3004::1:0)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 01 00 00"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(vlanID);

  // Cache the current stats
  CounterCache counters(sw.get());

  // We should get a neighbor solicitation back and the state should change once
  // to add the pending entry
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(1);
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:01:00:00"),
        IPAddressV6("ff02::1:ff01:0"),
        targetIP,
        vlanID));

  // Send the packet to the SwSwitch
  sw->packetReceived(pkt->clone());

  // Should see a pending entry now
  waitForStateUpdates(sw.get());
  auto entry = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()
    ->getEntryIf(targetIP);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  // Create a second packet to a node not in attached subnet, but in route table
  pkt = MockRxPacket::fromHex(
      // dst mac, src mac
      "02 01 02 03 04 05  02 05 73 f9 46 fc"
      // 802.1q, VLAN 5
      "81 00 00 05"
      // IPv6
      "86 dd"
      // Version 6, traffic class, flow label
      "6e 00 00 00"
      // Payload length: 24
      "00 18"
      // Next Header: 17 (UDP), Hop Limit (255)
      "11 ff"
      // src addr (2401:db00:2110:1234::1:0)
      "24 01 db 00 21 10 12 34 00 00 00 00 00 01 00 00"
      // dst addr (1111:1111:1:1:2:3:4:5)
      "11 11 11 11 00 01 00 01 00 02 00 03 00 04 00 05"
      // source port (53 - DNS)
      "00 35"
      // destination port (53 - DNS)
      "00 35"
      // length
      "00 00"
      // checksum (not valid)
      "2a 7e");
  pkt->padToLength(68);
  pkt->setSrcPort(PortID(1));
  pkt->setSrcVlan(VlanID(5));

  // We should send two more neighbor solicitations
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:01"),
        IPAddressV6("ff02::1:ff00:1"),
        targetIP2,
        VlanID(5)));

  EXPECT_PKT(sw, "neighbor solicitation", checkNeighborSolicitation(
        MacAddress("02:01:02:03:04:05"),
        IPAddressV6("fe80::0001:02ff:fe03:0405"),
        MacAddress("33:33:ff:00:00:02"),
        IPAddressV6("ff02::1:ff00:2"),
        targetIP3,
        VlanID(5)));

  // Send the packet to the SwSwitch
  sw->packetReceived(std::move(pkt));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);

  // Should see two more pending entries now
  waitForStateUpdates(sw.get());

  auto ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  auto entry2 = ndpTable->getEntryIf(targetIP2);
  auto entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), true);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), true);

  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sendNeighborAdvertisement(sw.get(), targetIP.str(),
                            "02:10:20:30:40:22", 1, vlanID);
  sendNeighborAdvertisement(sw.get(), targetIP2.str(),
                            "02:10:20:30:40:22", 1, vlanID);
  sendNeighborAdvertisement(sw.get(), targetIP3.str(),
                            "02:10:20:30:40:23", 2, vlanID);

  // Have getAndClearNeighborHit return false for the first entry,
  // but true for the others. This should result in the first
  // entry not being expired, while the others should get expired.
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP)))
    .WillRepeatedly(testing::Return(false));
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP2)))
    .WillRepeatedly(testing::Return(true));
  EXPECT_HW_CALL(sw, getAndClearNeighborHit(_, testing::Eq(targetIP3)))
    .WillRepeatedly(testing::Return(true));

  // The entries should now be valid instead of pending
  waitForStateUpdates(sw.get());
  ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  entry = ndpTable->getEntryIf(targetIP);
  entry2 = ndpTable->getEntryIf(targetIP2);
  entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), false);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), false);

  // send a port down event to the switch for port 2
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sw->linkStateChanged(PortID(1), false);
  // port down handling is async on the bg evb, so
  // block on something coming off of that
  waitForBackgroundThread(sw.get());
  waitForStateUpdates(sw.get());

  // The first two entries should be pending now, but not the third
  ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  entry = ndpTable->getEntryIf(targetIP);
  entry2 = ndpTable->getEntryIf(targetIP2);
  entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), true);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), false);

  // send a port up event to the switch for port 1
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  sw->linkStateChanged(PortID(1), true);

  sendNeighborAdvertisement(sw.get(), targetIP.str(),
                            "02:10:20:30:40:22", 1, vlanID);
  sendNeighborAdvertisement(sw.get(), targetIP2.str(),
                            "02:10:20:30:40:22", 1, vlanID);

  // All entries should be valid again
  waitForStateUpdates(sw.get());
  ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  entry = ndpTable->getEntryIf(targetIP);
  entry2 = ndpTable->getEntryIf(targetIP2);
  entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), false);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), false);
}
