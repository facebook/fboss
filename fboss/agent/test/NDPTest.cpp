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
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockRxPacket.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/NeighborEntryTest.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <netinet/icmp6.h>
#include <future>

using namespace facebook::fboss;
using facebook::network::toBinaryAddress;
using facebook::network::toIPAddress;
using facebook::network::thrift::BinaryAddress;
using folly::IOBuf;
using folly::IPAddress;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::StringPiece;
using folly::io::Cursor;
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;
using std::chrono::seconds;

using ::testing::_;

namespace {
// TODO(joseph5wu) Network control strict priority queue
const uint8_t kNCStrictPriorityQueue = 7;
const MacAddress kPlatformMac("02:01:02:03:04:05");

cfg::SwitchConfig createSwitchConfig(seconds raInterval, seconds ndpTimeout) {
  // Create a thrift config to use
  cfg::SwitchConfig config;
  config.vlans_ref()->resize(2);
  *config.vlans_ref()[0].name_ref() = "PrimaryVlan";
  *config.vlans_ref()[0].id_ref() = 5;
  *config.vlans_ref()[0].routable_ref() = true;
  config.vlans_ref()[0].intfID_ref() = 1234;
  *config.vlans_ref()[1].name_ref() = "DefaultHWVlan";
  *config.vlans_ref()[1].id_ref() = 1;
  *config.vlans_ref()[1].routable_ref() = true;
  config.vlans_ref()[1].intfID_ref() = 4321;

  config.vlanPorts_ref()->resize(10);
  config.ports_ref()->resize(10);
  for (int n = 0; n < 10; ++n) {
    *config.ports_ref()[n].logicalID_ref() = n + 1;
    *config.ports_ref()[n].state_ref() = cfg::PortState::ENABLED;
    *config.ports_ref()[n].minFrameSize_ref() = 64;
    *config.ports_ref()[n].maxFrameSize_ref() = 9000;
    *config.ports_ref()[n].routable_ref() = true;
    *config.ports_ref()[n].ingressVlan_ref() = 5;

    *config.vlanPorts_ref()[n].vlanID_ref() = 5;
    *config.vlanPorts_ref()[n].logicalPort_ref() = n + 1;
    *config.vlanPorts_ref()[n].spanningTreeState_ref() =
        cfg::SpanningTreeState::FORWARDING;
    *config.vlanPorts_ref()[n].emitTags_ref() = 0;
  }

  config.interfaces_ref()->resize(2);
  *config.interfaces_ref()[0].intfID_ref() = 1234;
  *config.interfaces_ref()[0].vlanID_ref() = 5;
  config.interfaces_ref()[0].name_ref() = "PrimaryInterface";
  config.interfaces_ref()[0].mtu_ref() = 9000;
  config.interfaces_ref()[0].ipAddresses_ref()->resize(5);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "10.164.4.10/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "10.164.4.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[2] = "10.164.4.2/24";
  config.interfaces_ref()[0].ipAddresses_ref()[3] = "2401:db00:2110:3004::/64";
  config.interfaces_ref()[0].ipAddresses_ref()[4] =
      "2401:db00:2110:3004::000a/64";
  config.interfaces_ref()[0].ndp_ref() = cfg::NdpConfig();
  *config.interfaces_ref()[0].ndp_ref()->routerAdvertisementSeconds_ref() =
      raInterval.count();
  *config.interfaces_ref()[1].intfID_ref() = 4321;
  *config.interfaces_ref()[1].vlanID_ref() = 1;
  config.interfaces_ref()[1].name_ref() = "DefaultHWInterface";
  config.interfaces_ref()[1].mtu_ref() = 9000;
  config.interfaces_ref()[1].ipAddresses_ref()->resize(0);

  if (ndpTimeout.count() > 0) {
    *config.arpTimeoutSeconds_ref() = ndpTimeout.count();
  }

  return config;
}

unique_ptr<HwTestHandle> setupTestHandle(
    seconds raInterval,
    seconds ndpInterval) {
  auto config = createSwitchConfig(raInterval, ndpInterval);

  *config.maxNeighborProbes_ref() = 1;
  *config.staleEntryInterval_ref() = 1;

  auto handle = createTestHandle(&config, kPlatformMac);
  handle->getSw()->initialConfigApplied(std::chrono::steady_clock::now());
  return handle;
}

shared_ptr<SwitchState> addMockRouteTable(shared_ptr<SwitchState> state) {
  RouteNextHopSet nexthops;
  // resolved by intf 1
  nexthops.emplace(UnresolvedNextHop(
      IPAddress("2401:db00:2110:3004::1"), UCMP_DEFAULT_WEIGHT));
  // resolved by intf 1
  nexthops.emplace(UnresolvedNextHop(
      IPAddress("2401:db00:2110:3004::2"), UCMP_DEFAULT_WEIGHT));
  // un-resolvable
  nexthops.emplace(UnresolvedNextHop(
      IPAddress("5555:db00:2110:3004::1"), UCMP_DEFAULT_WEIGHT));

  RouteUpdater updater(state->getRouteTables());
  updater.addRoute(
      RouterID(0),
      IPAddressV6("1111:1111:1:1::1"),
      64,
      ClientID(1001),
      RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));

  auto newRt = updater.updateDone();
  auto newState = state->clone();
  newState->resetRouteTables(newRt);
  return newState;
}

unique_ptr<HwTestHandle> setupTestHandle() {
  return setupTestHandle(seconds(0), seconds(0));
}

unique_ptr<HwTestHandle> setupTestHandleWithNdpTimeout(seconds ndpTimeout) {
  return setupTestHandle(seconds(0), ndpTimeout);
}

typedef std::function<void(Cursor* cursor, uint32_t length)> PayloadCheckFn;

TxMatchFn checkICMPv6Pkt(
    MacAddress srcMac,
    IPAddressV6 srcIP,
    MacAddress dstMac,
    IPAddressV6 dstIP,
    VlanID vlan,
    ICMPv6Type type,
    const PayloadCheckFn& checkPayload) {
  return [=](const TxPacket* pkt) {
    Cursor c(pkt->buf());
    auto parsedDstMac = PktUtil::readMac(&c);
    checkField(dstMac, parsedDstMac, "dst mac");
    auto parsedSrcMac = PktUtil::readMac(&c);
    checkField(srcMac, parsedSrcMac, "src mac");
    auto vlanType = c.readBE<uint16_t>();
    checkField(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN),
        vlanType,
        "VLAN ethertype");
    auto vlanTag = c.readBE<uint16_t>();
    checkField(static_cast<uint16_t>(vlan), vlanTag, "VLAN tag");
    auto ethertype = c.readBE<uint16_t>();
    checkField(
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6),
        ethertype,
        "ethertype");
    IPv6Hdr ipv6(c);
    checkField(
        static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP),
        ipv6.nextHeader,
        "IPv6 protocol");
    checkField(srcIP, ipv6.srcAddr, "src IP");
    checkField(dstIP, ipv6.dstAddr, "dst IP");

    Cursor ipv6PayloadStart(c);
    ICMPHdr icmp6(c);
    checkField(icmp6.computeChecksum(ipv6, c), icmp6.csum, "ICMPv6 checksum");
    checkField(static_cast<uint8_t>(type), icmp6.type, "ICMPv6 type");
    checkField(0, icmp6.code, "ICMPv6 code");

    checkPayload(&c, ipv6.payloadLength - ICMPHdr::SIZE);

    if (ipv6.payloadLength != (c - ipv6PayloadStart)) {
      throw FbossError(
          "IPv6 payload length mismatch: header says ",
          ipv6.payloadLength,
          " but we used ",
          c - ipv6PayloadStart);
    }

    // This is a match
    return;
  };
}

TxMatchFn checkNeighborAdvert(
    MacAddress srcMac,
    IPAddressV6 srcIP,
    MacAddress dstMac,
    IPAddressV6 dstIP,
    VlanID vlan,
    uint8_t flags) {
  auto checkPayload = [=](Cursor* cursor, uint32_t /*length*/) {
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
  return checkICMPv6Pkt(
      srcMac,
      srcIP,
      dstMac,
      dstIP,
      vlan,
      ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
      checkPayload);
}

TxMatchFn checkNeighborSolicitation(
    MacAddress srcMac,
    IPAddressV6 srcIP,
    MacAddress dstMac,
    IPAddressV6 dstIP,
    IPAddressV6 targetIP,
    VlanID vlan,
    bool hasOption = true) {
  auto checkPayload = [=](Cursor* cursor, uint32_t /*length*/) {
    auto reserved = cursor->read<uint32_t>();
    checkField(0, reserved, "NS reserved field");
    auto parsedTargetIP = PktUtil::readIPv6(cursor);
    checkField(targetIP, parsedTargetIP, "target IP");

    if (hasOption) {
      auto optionType = cursor->read<uint8_t>();
      checkField(1, optionType, "source MAC option type");
      auto optionLength = cursor->read<uint8_t>();
      checkField(1, optionLength, "source MAC option length");
      auto srcMacOption = PktUtil::readMac(cursor);
      checkField(srcMac, srcMacOption, "source MAC option value");
    }
  };
  return checkICMPv6Pkt(
      srcMac,
      srcIP,
      dstMac,
      dstIP,
      vlan,
      ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
      checkPayload);
}

typedef std::vector<std::pair<IPAddressV6, uint8_t>> PrefixVector;
TxMatchFn checkRouterAdvert(
    MacAddress srcMac,
    IPAddressV6 srcIP,
    MacAddress dstMac,
    IPAddressV6 dstIP,
    VlanID vlan,
    const cfg::NdpConfig& ndp,
    uint32_t mtu,
    PrefixVector expectedPrefixes) {
  auto checkPayload = [=](Cursor* cursor, uint32_t length) {
    Cursor start(*cursor);
    Cursor end = start + length;
    auto parsedHopLimit = cursor->read<uint8_t>();
    checkField(*ndp.curHopLimit_ref(), parsedHopLimit, "cur hop limit");
    auto parsedFlags = cursor->read<uint8_t>();
    checkField(0, parsedFlags, "NDP RA flags");
    auto parsedLifetime = cursor->readBE<uint16_t>();
    checkField(*ndp.routerLifetime_ref(), parsedLifetime, "router lifetime");
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
          checkField(
              *ndp.prefixValidLifetimeSeconds_ref(),
              prefixValidLifetime,
              "prefix valid lifetime");
          auto prefixPreferredLifetime = cursor->readBE<uint32_t>();
          checkField(
              *ndp.prefixPreferredLifetimeSeconds_ref(),
              prefixPreferredLifetime,
              "prefix preferred lifetime");
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
          throw FbossError(
              "unexpected NDP option type ",
              optionType,
              " at payload offset ",
              (*cursor - start) - 2,
              "\n",
              PktUtil::hexDump(start));
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
  return checkICMPv6Pkt(
      srcMac,
      srcIP,
      dstMac,
      dstIP,
      vlan,
      ICMPv6Type::ICMPV6_TYPE_NDP_ROUTER_ADVERTISEMENT,
      checkPayload);
}

void sendNeighborAdvertisement(
    HwTestHandle* handle,
    StringPiece ipStr,
    StringPiece macStr,
    int port,
    int vlanID,
    bool solicited = true) {
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
  ipv6.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  ipv6.hopLimit = 255;

  size_t totalLen = EthHdr::SIZE + IPv6Hdr::SIZE + ipv6.payloadLength;
  auto buf = IOBuf::create(totalLen);
  buf->append(totalLen);
  folly::io::RWPrivateCursor cursor(buf.get());

  auto bodyFn = [&](folly::io::RWPrivateCursor* c) {
    c->write<uint32_t>(
        ND_NA_FLAG_OVERRIDE | (solicited ? ND_NA_FLAG_SOLICITED : 0));
    c->push(srcIP.bytes(), IPAddressV6::byteCount());
  };

  ICMPHdr icmp6(
      static_cast<uint8_t>(ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT),
      0,
      0);
  icmp6.serializeFullPacket(&cursor, dstMac, srcMac, vlan, ipv6, plen, bodyFn);

  // Send the packet to the switch
  PktUtil::padToLength(buf.get(), totalLen);
  handle->rxPacket(std::move(buf), PortID(port), vlan);
}

} // unnamed namespace

TEST(NdpTest, UnsolicitedRequest) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  // Create an neighbor solicitation request
  auto pkt = PktUtil::parseHexData(
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
      // dst addr (2401:db00:2110:3004::a)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 00 00 0a"
      // type: neighbor solicitation
      "87"
      // code
      "00"
      // checksum
      "d8 6c"
      // reserved
      "00 00 00 00"
      // target address (2401:db00:2110:3004::a)
      "24 01 db 00 21 10 30 04 00 00 00 00 00 00 00 0a");

  // Cache the current stats
  CounterCache counters(sw);

  // We should get a neighbor advertisement back
  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "neighbor advertisement",
      checkNeighborAdvert(
          kPlatformMac,
          IPAddressV6("2401:db00:2110:3004::a"),
          MacAddress("02:05:73:f9:46:fc"),
          IPAddressV6("ff01::1"),
          VlanID(5),
          0xa0),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), VlanID(5));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 1);
}

TEST(NdpTest, TriggerSolicitation) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  sw->updateStateBlocking("add test route table", addMockRouteTable);

  // Create a packet to a node in the attached IPv6 subnet
  auto pkt = PktUtil::parseHexData(
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

  // Cache the current stats
  CounterCache counters(sw);

  // We should get a neighbor solicitation back
  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:01:00:00"),
          IPAddressV6("ff02::1:ff01:0"),
          IPAddressV6("2401:db00:2110:3004::1:0"),
          VlanID(5)));

  WaitForNdpEntryCreation neighborEntryCreate(
      sw, IPAddressV6("2401:db00:2110:3004::1:0"), VlanID(5));
  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), VlanID(5));

  // expect neighbor solicitation to neighbor in the subnet & entry in NDP table
  EXPECT_TRUE(neighborEntryCreate.wait());

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);

  // Create a packet to a node not in attached subnet, but in route table
  pkt = PktUtil::parseHexData(
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

  // We should get a neighbor solicitation back
  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:00:00:01"),
          IPAddressV6("ff02::1:ff00:1"),
          IPAddressV6("2401:db00:2110:3004::1"),
          VlanID(5)));

  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:00:00:02"),
          IPAddressV6("ff02::1:ff00:2"),
          IPAddressV6("2401:db00:2110:3004::2"),
          VlanID(5)));

  /* expect neighbor solicitations to nexthops and their entries in NDP table */
  WaitForNdpEntryCreation nextHop1Create(
      sw, IPAddressV6("2401:db00:2110:3004::1"), VlanID(5));
  WaitForNdpEntryCreation nextHop2Create(
      sw, IPAddressV6("2401:db00:2110:3004::2"), VlanID(5));

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), VlanID(5));

  EXPECT_TRUE(nextHop1Create.wait());
  EXPECT_TRUE(nextHop2Create.wait());

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
}

TEST(NdpTest, RouterAdvertisement) {
  seconds raInterval(1);
  auto config = createSwitchConfig(raInterval, seconds(0));
  // Add an interface with a /128 mask, to make sure it isn't included
  // in the generated RA packets.
  config.interfaces_ref()[0].ipAddresses_ref()->push_back(
      "2401:db00:2000:1234:1::/128");
  auto handle = createTestHandle(&config, kPlatformMac);
  auto sw = handle->getSw();
  sw->initialConfigApplied(std::chrono::steady_clock::now());

  auto state = sw->getState();
  auto intfConfig = state->getInterfaces()->getInterface(InterfaceID(1234));
  PrefixVector expectedPrefixes{
      {IPAddressV6("2401:db00:2110:3004::"), 64},
      {IPAddressV6("fe80::"), 64},
  };
  // Multicast router advertisement use switched api
  EXPECT_SWITCHED_PKT(
      sw,
      "router advertisement",
      checkRouterAdvert(
          kPlatformMac,
          IPAddressV6("fe80::1:02ff:fe03:0405"),
          MacAddress("33:33:00:00:00:01"),
          IPAddressV6("ff02::1"),
          VlanID(5),
          intfConfig->getNdpConfig(),
          9000,
          expectedPrefixes));

  // Send the router solicitation packet
  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "router advertisement",
      checkRouterAdvert(
          kPlatformMac,
          IPAddressV6("fe80::1:02ff:fe03:0405"),
          MacAddress("02:05:73:f9:46:fc"),
          IPAddressV6("2401:db00:2110:1234::1:0"),
          VlanID(5),
          intfConfig->getNdpConfig(),
          9000,
          expectedPrefixes),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));

  auto pkt = PktUtil::parseHexData(
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
      "00 00 00 00");
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), VlanID(5));

  // Now send the packet with specified source MAC address as ICMPv6 option
  // which differs from MAC address in ethernet header. The switch should use
  // the MAC address in options field.

  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "router advertisement",
      checkRouterAdvert(
          kPlatformMac,
          IPAddressV6("fe80::1:02ff:fe03:0405"),
          MacAddress("02:ab:73:f9:46:fc"),
          IPAddressV6("2401:db00:2110:1234::1:0"),
          VlanID(5),
          intfConfig->getNdpConfig(),
          9000,
          expectedPrefixes),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));

  auto pkt2 = PktUtil::parseHexData(
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
      "02 ab 73 f9 46 fc");
  handle->rxPacket(make_unique<IOBuf>(pkt2), PortID(1), VlanID(5));

  // The RA packet will be sent in the background even thread after the RA
  // interval.  Schedule a timeout to wake us up after the interval has
  // expired.  Using the background EventBase to run the timeout ensures that
  // it will always run after the RA timeout has fired.
  std::promise<bool> done;
  auto* evb = sw->getBackgroundEvb();
  evb->runInEventBaseThread(
      [&]() { evb->tryRunAfterDelay([&]() { done.set_value(true); }, 1010); });
  done.get_future().wait();
  // We send RA packets just before switch controller shutdown,
  // so expect RA packet.
  // Multicast router advertisement use switched api
  EXPECT_SWITCHED_PKT(
      sw,
      "router advertisement",
      checkRouterAdvert(
          kPlatformMac,
          IPAddressV6("fe80::1:02ff:fe03:0405"),
          MacAddress("33:33:00:00:00:01"),
          IPAddressV6("ff02::1"),
          VlanID(5),
          intfConfig->getNdpConfig(),
          9000,
          expectedPrefixes));
}

TEST(NdpTest, receiveNeighborAdvertisementUnsolicited) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  // Send two unsolicited neighbor advertisements, state should update at
  // least once
  WaitForNdpEntryCreation neighbor1Create(
      sw, IPAddressV6("2401:db00:2110:3004::b"), VlanID(5));

  sendNeighborAdvertisement(
      handle.get(), "2401:db00:2110:3004::b", "02:05:73:f9:46:fb", 1, 5, false);
  EXPECT_TRUE(neighbor1Create.wait());
  ThriftHandler thriftHandler(sw);
  auto binAddr = toBinaryAddress(IPAddressV6("2401:db00:2110:3004::b"));
  auto numFlushed =
      thriftHandler.flushNeighborEntry(make_unique<BinaryAddress>(binAddr), 5);
  EXPECT_EQ(numFlushed, 1);
}

TEST(NdpTest, FlushEntry) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  ThriftHandler thriftHandler(sw);

  // Helper for checking entries in NDP table
  auto getNDPTableEntry = [&](IPAddressV6 ip, VlanID vlan) {
    return sw->getState()
        ->getVlans()
        ->getVlanIf(vlan)
        ->getNdpTable()
        ->getEntryIf(ip);
  };

  // Send two unsolicited neighbor advertisements, state should update at
  // least once
  WaitForNdpEntryCreation neighbor1Create(
      sw, IPAddressV6("2401:db00:2110:3004::b"), VlanID(5));
  WaitForNdpEntryCreation neighbor2Create(
      sw, IPAddressV6("2401:db00:2110:3004::c"), VlanID(5));

  sendNeighborAdvertisement(
      handle.get(), "2401:db00:2110:3004::b", "02:05:73:f9:46:fb", 1, 5);
  sendNeighborAdvertisement(
      handle.get(), "2401:db00:2110:3004::c", "02:05:73:f9:46:fc", 1, 5);

  EXPECT_TRUE(neighbor1Create.wait());
  EXPECT_TRUE(neighbor2Create.wait());

  // Flush 2401:db00:2110:3004::b
  WaitForNdpEntryExpiration neighbor1Expire(
      sw, IPAddressV6("2401:db00:2110:3004::b"), VlanID(5));
  auto binAddr = toBinaryAddress(IPAddressV6("2401:db00:2110:3004::b"));
  auto numFlushed =
      thriftHandler.flushNeighborEntry(make_unique<BinaryAddress>(binAddr), 5);
  EXPECT_EQ(numFlushed, 1);
  EXPECT_TRUE(neighbor1Expire.wait());

  // Still one entry
  // Only one entry now
  auto entry0 =
      getNDPTableEntry(IPAddressV6("2401:db00:2110:3004::b"), VlanID(5));
  EXPECT_EQ(entry0, nullptr);

  auto entry1 =
      getNDPTableEntry(IPAddressV6("2401:db00:2110:3004::c"), VlanID(5));
  EXPECT_NE(entry1, nullptr);
  EXPECT_EQ(entry1->getMac(), MacAddress("02:05:73:f9:46:fc"));

  // Now flush 2401:db00:2110:3004::c, but using special Vlan0
  WaitForNdpEntryExpiration neighbor2Expire(
      sw, IPAddressV6("2401:db00:2110:3004::c"), VlanID(5));
  binAddr = toBinaryAddress(IPAddressV6("2401:db00:2110:3004::c"));
  // NDP removal should trigger a static MAC entry removal
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(2);
  numFlushed =
      thriftHandler.flushNeighborEntry(make_unique<BinaryAddress>(binAddr), 0);
  EXPECT_EQ(numFlushed, 1);
  EXPECT_TRUE(neighbor2Expire.wait());
  waitForStateUpdates(sw);

  // Try flushing 2401:db00:2110:3004::c again (should be a no-op)
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);
  binAddr = toBinaryAddress(IPAddressV6("2401:db00:2110:3004::c"));
  numFlushed =
      thriftHandler.flushNeighborEntry(make_unique<BinaryAddress>(binAddr), 5);
  EXPECT_EQ(numFlushed, 0);
}

TEST(NdpTest, PendingNdp) {
  auto handle = setupTestHandle();
  auto sw = handle->getSw();

  auto vlanID = VlanID(5);

  // Create a packet to a node in the attached IPv6 subnet
  auto pkt = PktUtil::parseHexData(
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

  // Cache the current stats
  CounterCache counters(sw);

  // We should get a neighbor solicitation back and the state should change once
  // to add the pending entry
  WaitForNdpEntryCreation neighborEntryCreate(
      sw, IPAddressV6("2401:db00:2110:3004::1:0"), vlanID);
  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:01:00:00"),
          IPAddressV6("ff02::1:ff01:0"),
          IPAddressV6("2401:db00:2110:3004::1:0"),
          vlanID));

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), vlanID);
  EXPECT_TRUE(neighborEntryCreate.wait());

  // Should see a pending entry now
  auto entry =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  WaitForNdpEntryReachable neighborEntryReachable(
      sw, IPAddressV6("2401:db00:2110:3004::1:0"), vlanID);

  // Receive an ndp advertisement for our pending entry
  sendNeighborAdvertisement(
      handle.get(), "2401:db00:2110:3004::1:0", "02:10:20:30:40:22", 1, vlanID);

  // The entry should now be valid instead of pending
  EXPECT_TRUE(neighborEntryReachable.wait());
  waitForStateUpdates(sw);
  entry =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);

  // Verify that we don't ever overwrite a valid entry with a pending one.
  // Receive the same packet again, entry should still be valid
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(0);

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), vlanID);
  waitForStateUpdates(sw);
  entry =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), false);
};

TEST(NdpTest, PendingNdpCleanup) {
  seconds ndpTimeout(1);
  auto handle = setupTestHandleWithNdpTimeout(ndpTimeout);
  auto sw = handle->getSw();

  sw->updateStateBlocking("add test route table", addMockRouteTable);

  auto vlanID = VlanID(5);

  // Create a packet to a node in the attached IPv6 subnet
  auto pkt = PktUtil::parseHexData(
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

  // Cache the current stats
  CounterCache counters(sw);

  // We should get a neighbor solicitation back and the state should change once
  // to add the pending entry
  WaitForNdpEntryCreation neighborEntryCreate(
      sw, IPAddressV6("2401:db00:2110:3004::1:0"), vlanID);
  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:01:00:00"),
          IPAddressV6("ff02::1:ff01:0"),
          IPAddressV6("2401:db00:2110:3004::1:0"),
          vlanID));

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), vlanID);
  // waitForStateUpdates(sw);
  // Should see a pending entry now
  EXPECT_TRUE(neighborEntryCreate.wait());
  auto entry =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          IPAddressV6("2401:db00:2110:3004::1:0"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  // Create a second packet to a node not in attached subnet, but in route table
  pkt = PktUtil::parseHexData(
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

  // We should send two more neighbor solicitations
  WaitForNdpEntryCreation nexthop1Create(
      sw, IPAddressV6("2401:db00:2110:3004::1"), vlanID);
  WaitForNdpEntryCreation nexthop2Create(
      sw, IPAddressV6("2401:db00:2110:3004::2"), vlanID);

  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:00:00:01"),
          IPAddressV6("ff02::1:ff00:1"),
          IPAddressV6("2401:db00:2110:3004::1"),
          VlanID(5)));

  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:00:00:02"),
          IPAddressV6("ff02::1:ff00:2"),
          IPAddressV6("2401:db00:2110:3004::2"),
          VlanID(5)));

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), VlanID(5));

  EXPECT_TRUE(nexthop1Create.wait());
  EXPECT_TRUE(nexthop2Create.wait());

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);

  // Should see two more pending entries now
  entry =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          IPAddressV6("2401:db00:2110:3004::1"));
  auto entry2 =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          IPAddressV6("2401:db00:2110:3004::2"));
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), true);

  // Wait for pending entries to expire
  WaitForNdpEntryExpiration neighborExpire(
      sw, IPAddressV6("2401:db00:2110:3004::1:0"), vlanID);
  WaitForNdpEntryExpiration nexthop1Expire(
      sw, IPAddressV6("2401:db00:2110:3004::1"), vlanID);
  WaitForNdpEntryExpiration nexthop2Expire(
      sw, IPAddressV6("2401:db00:2110:3004::2"), vlanID);

  std::promise<bool> done;
  auto* evb = sw->getBackgroundEvb();
  evb->runInEventBaseThread(
      [&]() { evb->tryRunAfterDelay([&]() { done.set_value(true); }, 1050); });
  done.get_future().wait();

  EXPECT_TRUE(neighborExpire.wait());
  EXPECT_TRUE(nexthop1Expire.wait());
  EXPECT_TRUE(nexthop2Expire.wait());
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
  auto handle = setupTestHandleWithNdpTimeout(ndpTimeout);
  auto sw = handle->getSw();

  sw->updateStateBlocking("add test route table", addMockRouteTable);

  auto vlanID = VlanID(5);

  auto targetIP = IPAddressV6("2401:db00:2110:3004::1:0");
  auto targetIP2 = IPAddressV6("2401:db00:2110:3004::1");
  auto targetIP3 = IPAddressV6("2401:db00:2110:3004::2");

  // Create a packet to a node in the attached IPv6 subnet
  auto pkt = PktUtil::parseHexData(
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

  // Cache the current stats
  CounterCache counters(sw);

  // We should get a neighbor solicitation back and the state should change once
  // to add the pending entry
  WaitForNdpEntryCreation neighbor0Create(sw, targetIP, vlanID);
  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:01:00:00"),
          IPAddressV6("ff02::1:ff01:0"),
          targetIP,
          vlanID));
  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), vlanID);

  // Should see a pending entry now
  EXPECT_TRUE(neighbor0Create.wait());
  auto entry =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          targetIP);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  // Create a second packet to a node not in attached subnet, but in route table
  pkt = PktUtil::parseHexData(
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

  // We should send two more neighbor solicitations
  WaitForNdpEntryCreation neighbor1Create(sw, targetIP2, vlanID);
  WaitForNdpEntryCreation neighbor2Create(sw, targetIP3, vlanID);

  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:00:00:01"),
          IPAddressV6("ff02::1:ff00:1"),
          targetIP2,
          VlanID(5)));

  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:00:00:02"),
          IPAddressV6("ff02::1:ff00:2"),
          targetIP3,
          VlanID(5)));

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), VlanID(5));

  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);

  // Should see two more pending entries now
  EXPECT_TRUE(neighbor1Create.wait());
  EXPECT_TRUE(neighbor2Create.wait());

  auto ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  auto entry2 = ndpTable->getEntryIf(targetIP2);
  auto entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), true);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), true);

  WaitForNdpEntryReachable neighbor0Reachable(sw, targetIP, vlanID);
  WaitForNdpEntryReachable neighbor1Reachable(sw, targetIP2, vlanID);
  WaitForNdpEntryReachable neighbor2Reachable(sw, targetIP3, vlanID);

  // Receive ndp advertisements for our pending entries
  sendNeighborAdvertisement(
      handle.get(), targetIP.str(), "02:10:20:30:40:22", 1, vlanID);
  sendNeighborAdvertisement(
      handle.get(), targetIP2.str(), "02:10:20:30:40:23", 1, vlanID);
  sendNeighborAdvertisement(
      handle.get(), targetIP3.str(), "02:10:20:30:40:24", 1, vlanID);

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
  EXPECT_TRUE(neighbor0Reachable.wait());
  EXPECT_TRUE(neighbor1Reachable.wait());
  EXPECT_TRUE(neighbor2Reachable.wait());

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

  // before we expire them, but this time they're unicast as they're being
  // probed
  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("2401:db00:2110:3004::"),
          MacAddress("02:10:20:30:40:23"),
          targetIP2,
          targetIP2,
          VlanID(5),
          false),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));

  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("2401:db00:2110:3004::"),
          MacAddress("02:10:20:30:40:24"),
          targetIP3,
          targetIP3,
          VlanID(5),
          false),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));

  // Wait for the second and third entries to expire.
  // We wait 2.5 seconds(plus change):
  // Up to 1.5 seconds for lifetime.
  // 1 more second for probe
  WaitForNdpEntryExpiration expire1(sw, targetIP2, vlanID);
  WaitForNdpEntryExpiration expire2(sw, targetIP3, vlanID);
  std::promise<bool> done;
  auto* evb = sw->getBackgroundEvb();
  evb->runInEventBaseThread(
      [&]() { evb->tryRunAfterDelay([&]() { done.set_value(true); }, 2550); });
  done.get_future().wait();
  EXPECT_TRUE(expire1.wait());
  EXPECT_TRUE(expire2.wait());

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

  EXPECT_OUT_OF_PORT_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("2401:db00:2110:3004::"),
          MacAddress("02:10:20:30:40:22"),
          targetIP,
          targetIP,
          vlanID,
          false),
      PortID(1),
      std::optional<uint8_t>(kNCStrictPriorityQueue));
  // Wait for the first entry to expire
  WaitForNdpEntryExpiration expire0(sw, targetIP, vlanID);
  std::promise<bool> done2;
  evb->runInEventBaseThread(
      [&]() { evb->tryRunAfterDelay([&]() { done2.set_value(true); }, 2050); });
  done2.get_future().wait();
  EXPECT_TRUE(expire0.wait());

  // First entry should now be expired
  entry =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          targetIP);
  EXPECT_EQ(entry, nullptr);
}

TEST(NdpTest, PortFlapRecover) {
  seconds ndpTimeout(1);
  auto handle = setupTestHandleWithNdpTimeout(ndpTimeout);
  auto sw = handle->getSw();

  sw->updateStateBlocking("add test route table", addMockRouteTable);

  // We only have special port down handling after the fib is
  // synced, so we set that here.
  sw->fibSynced();

  // ensure port is up
  sw->linkStateChanged(PortID(1), true);

  auto vlanID = VlanID(5);

  auto targetIP = IPAddressV6("2401:db00:2110:3004::1:0");
  auto targetIP2 = IPAddressV6("2401:db00:2110:3004::1");
  auto targetIP3 = IPAddressV6("2401:db00:2110:3004::2");

  // Create a packet to a node in the attached IPv6 subnet
  auto pkt = PktUtil::parseHexData(
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

  // Cache the current stats
  CounterCache counters(sw);

  // We should get a neighbor solicitation back and the state should change once
  // to add the pending entry
  WaitForNdpEntryCreation neighbor0Create(sw, targetIP, vlanID);
  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:01:00:00"),
          IPAddressV6("ff02::1:ff01:0"),
          targetIP,
          vlanID));

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), vlanID);

  // Should see a pending entry now
  EXPECT_TRUE(neighbor0Create.wait());
  auto entry =
      sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable()->getEntryIf(
          targetIP);
  EXPECT_NE(entry, nullptr);
  EXPECT_EQ(entry->isPending(), true);

  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.ndp.sum", 0);

  // Create a second packet to a node not in attached subnet, but in route table
  pkt = PktUtil::parseHexData(
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

  // We should send two more neighbor solicitations
  WaitForNdpEntryCreation neighbor1Create(sw, targetIP2, vlanID);
  WaitForNdpEntryCreation neighbor2Create(sw, targetIP3, vlanID);
  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:00:00:01"),
          IPAddressV6("ff02::1:ff00:1"),
          targetIP2,
          VlanID(5)));

  EXPECT_SWITCHED_PKT(
      sw,
      "neighbor solicitation",
      checkNeighborSolicitation(
          MacAddress("02:01:02:03:04:05"),
          IPAddressV6("fe80::0001:02ff:fe03:0405"),
          MacAddress("33:33:ff:00:00:02"),
          IPAddressV6("ff02::1:ff00:2"),
          targetIP3,
          VlanID(5)));

  // Send the packet to the SwSwitch
  handle->rxPacket(make_unique<IOBuf>(pkt), PortID(1), VlanID(5));

  EXPECT_TRUE(neighbor1Create.wait());
  EXPECT_TRUE(neighbor2Create.wait());
  // Check the new stats
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "trapped.pkts.sum", 1);

  // Should see two more pending entries now

  auto ndpTable = sw->getState()->getVlans()->getVlanIf(vlanID)->getNdpTable();
  auto entry2 = ndpTable->getEntryIf(targetIP2);
  auto entry3 = ndpTable->getEntryIf(targetIP3);
  EXPECT_NE(entry2, nullptr);
  EXPECT_EQ(entry2->isPending(), true);
  EXPECT_NE(entry3, nullptr);
  EXPECT_EQ(entry3->isPending(), true);

  WaitForNdpEntryReachable neighbor0Reachable(sw, targetIP, vlanID);
  WaitForNdpEntryReachable neighbor1Reachable(sw, targetIP2, vlanID);
  WaitForNdpEntryReachable neighbor2Reachable(sw, targetIP3, vlanID);

  sendNeighborAdvertisement(
      handle.get(), targetIP.str(), "02:10:20:30:40:22", 1, vlanID);
  sendNeighborAdvertisement(
      handle.get(), targetIP2.str(), "02:10:20:30:40:22", 1, vlanID);
  sendNeighborAdvertisement(
      handle.get(), targetIP3.str(), "02:10:20:30:40:23", 2, vlanID);

  EXPECT_TRUE(neighbor0Reachable.wait());
  EXPECT_TRUE(neighbor1Reachable.wait());
  EXPECT_TRUE(neighbor2Reachable.wait());

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

  // send a port down event to the switch for port 1
  EXPECT_HW_CALL(sw, stateChanged(_)).Times(testing::AtLeast(1));
  WaitForNdpEntryPending neigbor0Pending(sw, targetIP, vlanID);
  WaitForNdpEntryPending neigbor1Pending(sw, targetIP2, vlanID);

  sw->linkStateChanged(PortID(1), false);

  // purging neighbor entries occurs on the background EVB via NeighorUpdater as
  // a StateObserver.
  // block until NeighborUpdater::stateChanged() has been invoked
  waitForStateUpdates(sw);
  // block until neighbor purging logic has been executed on the background evb
  waitForBackgroundThread(sw);
  // block until updates to neighbor entries have been picked up by SwitchState
  waitForStateUpdates(sw);

  // The first two entries should be pending now, but not the third
  EXPECT_TRUE(neigbor0Pending.wait());
  EXPECT_TRUE(neigbor1Pending.wait());

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

  sendNeighborAdvertisement(
      handle.get(), targetIP.str(), "02:10:20:30:40:22", 1, vlanID);
  sendNeighborAdvertisement(
      handle.get(), targetIP2.str(), "02:10:20:30:40:22", 1, vlanID);

  EXPECT_TRUE(neighbor0Reachable.wait());
  EXPECT_TRUE(neighbor1Reachable.wait());

  // All entries should be valid again
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
