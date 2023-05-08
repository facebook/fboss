/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/DHCPv6Handler.h"
#include <arpa/inet.h>
#include <folly/IPAddressV6.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>
#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/DHCPv6Packet.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

using folly::IOBuf;
using folly::IPAddress;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::string;
using std::unique_ptr;
using namespace facebook::fboss;

typedef EthHdr::VlanTags_t VlanTags_t;

namespace {

template <typename DHCPBodyFn>
void sendDHCPv6Packet(
    SwSwitch* sw,
    MacAddress dstMac,
    MacAddress srcMac,
    std::optional<VlanID> vlan,
    IPAddressV6 dstIp,
    IPAddressV6 srcIp,
    uint16_t udpDstPort,
    uint16_t udpSrcPort,
    uint32_t dhcpLength,
    DHCPBodyFn serializeDhcp) {
  // construct EthHdr,
  VlanTags_t vlanTags;
  if (vlan.has_value()) {
    vlanTags.push_back(VlanTag(
        vlan.value(), static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN)));
  }

  EthHdr ethHdr(
      dstMac,
      srcMac,
      vlanTags,
      static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));

  // IPv6Hdr
  IPv6Hdr ipHdr(srcIp, dstIp);
  ipHdr.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP);
  ipHdr.trafficClass = kGetNetworkControlTrafficClass();
  ipHdr.payloadLength = UDPHeader::size() + dhcpLength;
  ipHdr.hopLimit = 255;

  // UDPHeader
  UDPHeader udpHdr(udpSrcPort, udpDstPort, UDPHeader::size() + dhcpLength);

  // Allocate packet
  auto txPacket = sw->allocatePacket(
      18 + // ethernet header
      IPv6Hdr::SIZE + udpHdr.size() + dhcpLength);

  RWPrivateCursor rwCursor(txPacket->buf());
  // Write EthHdr
  txPacket->writeEthHeader(
      &rwCursor,
      ethHdr.getDstMac(),
      ethHdr.getSrcMac(),
      VlanID(vlanTags[0].vid()),
      ethHdr.getEtherType());
  ipHdr.serialize(&rwCursor);

  // write UDP header, DHCP packet and compute checksum
  rwCursor.writeBE<uint16_t>(udpHdr.srcPort);
  rwCursor.writeBE<uint16_t>(udpHdr.dstPort);
  rwCursor.writeBE<uint16_t>(udpHdr.length);
  folly::io::RWPrivateCursor csumCursor(rwCursor);
  rwCursor.skip(2);
  folly::io::Cursor payloadStart(rwCursor);
  serializeDhcp(&rwCursor);
  udpHdr.updateChecksum(ipHdr, payloadStart);
  csumCursor.writeBE<uint16_t>(udpHdr.csum);

  XLOG(DBG4) << " Send dhcp packet:"
             << " Eth header: " << ethHdr.toString()
             << " IP header: " << ipHdr.toString()
             << " UDP Header: " << udpHdr.toString()
             << " dhcpLength: " << dhcpLength;
  // Send packet
  sw->sendPacketSwitchedAsync(std::move(txPacket));
}

} // namespace

namespace facebook::fboss {

bool DHCPv6Handler::isForDHCPv6RelayOrServer(const UDPHeader& udpHdr) {
  // according to RFC 3315 section 5.2, packets to server or agent are
  // all on port 547
  return (udpHdr.dstPort == DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT);
}

void DHCPv6Handler::handlePacket(
    SwSwitch* sw,
    std::unique_ptr<RxPacket> pkt,
    MacAddress srcMac,
    MacAddress dstMac,
    const IPv6Hdr& ipHdr,
    const UDPHeader& /*udpHdr*/,
    Cursor cursor) {
  sw->portStats(pkt->getSrcPort())->dhcpV6Pkt();
  // Parse dhcp packet
  DHCPv6Packet dhcp6Pkt;
  try {
    dhcp6Pkt.parse(&cursor);
  } catch (const FbossError& err) {
    sw->portStats(pkt->getSrcPort())->dhcpV6BadPkt();
    throw; // Rethrow
  }
  if (dhcp6Pkt.type == static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD)) {
    XLOG(DBG4) << "Received DHCPv6 relay forward packet: "
               << dhcp6Pkt.toString();
    processDHCPv6RelayForward(
        sw, std::move(pkt), srcMac, dstMac, ipHdr, dhcp6Pkt);
  } else if (
      dhcp6Pkt.type == static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_REPLY)) {
    XLOG(DBG4) << "Received DHCPv6 relay reply packet: " << dhcp6Pkt.toString();
    processDHCPv6RelayReply(
        sw, std::move(pkt), srcMac, dstMac, ipHdr, dhcp6Pkt);
  } else {
    XLOG(DBG4) << "Received DHCPv6 packet: " << dhcp6Pkt.toString();
    processDHCPv6Packet(sw, std::move(pkt), srcMac, dstMac, ipHdr, dhcp6Pkt);
  }
}

void DHCPv6Handler::processDHCPv6Packet(
    SwSwitch* sw,
    std::unique_ptr<RxPacket> pkt,
    MacAddress srcMac,
    MacAddress /*dstMac*/,
    const IPv6Hdr& ipHdr,
    const DHCPv6Packet& dhcpPacket) {
  auto vlanId = pkt->getSrcVlanIf();
  auto vlanIdStr = vlanId.has_value()
      ? folly::to<std::string>(static_cast<int>(vlanId.value()))
      : "None";
  auto states = sw->getState();
  auto vlan = states->getVlans()->getVlanIf(sw->getVlanIDHelper(vlanId));
  if (!vlan) {
    sw->stats()->dhcpV6DropPkt();
    XLOG(DBG2) << "VLAN " << vlanIdStr << " is no longer present"
               << "DHCPv6Packet dropped.";
    return;
  }

  auto dhcp6ServerIp = vlan->getDhcpV6Relay();

  // look in the override map, and use relevant destination
  XLOG(DBG4) << "srcMac: " << srcMac.toString();
  auto dhcpOverrideMap = vlan->getDhcpV6RelayOverrides();
  for (auto o : dhcpOverrideMap) {
    if (MacAddress(o.first) == srcMac) {
      dhcp6ServerIp = o.second;
      XLOG(DBG4) << "dhcp6ServerIp: " << dhcp6ServerIp;
      break;
    }
  }

  if (dhcp6ServerIp.isZero()) {
    XLOG(DBG4) << "No DHCPv6 relay configured for Vlan " << vlanIdStr
               << " dropped DHCPv6 packet";
    sw->stats()->dhcpV6DropPkt();
    return;
  }

  auto switchIp = states->getDhcpV6RelaySrc();
  if (switchIp.isZero()) {
    switchIp = getSwitchIntfIPv6(
        states, sw->getState()->getInterfaceIDForPort(pkt->getSrcPort()));
  }

  // link address set to unspecified
  IPAddressV6 la("::");
  // ip src -> peer-address
  IPAddressV6 pa = ipHdr.srcAddr;
  DHCPv6Packet relayFwdPkt(
      static_cast<uint8_t>(DHCPv6Type::DHCPv6_RELAY_FORWARD), 0, la, pa);

  // use the client src mac address as the interface id
  relayFwdPkt.addInterfaceIDOption(srcMac);
  // add relay message option
  relayFwdPkt.addRelayMessageOption(dhcpPacket);

  if (relayFwdPkt.computePacketLength() > DHCPv6Packet::MAX_DHCPV6_MSG_LENGTH) {
    XLOG(DBG2) << "DHCPv6 relay forward message exceeds max length, drop it.";
    sw->portStats(pkt->getSrcPort())->dhcpV6BadPkt();
    return;
  }

  // create the dhcpv6 packet
  // vlanIp -> ip src, ipHdr.dst -> ip dst, srcMac -> mac src, dstMac -> mac dst
  auto switchId = sw->getScopeResolver()->scope(vlan).switchId();
  MacAddress cpuMac = sw->getHwAsicTable()->getHwAsicIf(switchId)->getAsicMac();
  auto serializeBody = [&](RWPrivateCursor* sendCursor) {
    relayFwdPkt.write(sendCursor);
  };

  sendDHCPv6Packet(
      sw,
      cpuMac,
      cpuMac,
      vlanId,
      dhcp6ServerIp,
      switchIp,
      DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT,
      DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT,
      relayFwdPkt.computePacketLength(),
      serializeBody);
}

void DHCPv6Handler::processDHCPv6RelayForward(
    SwSwitch* sw,
    std::unique_ptr<RxPacket> pkt,
    MacAddress srcMac,
    MacAddress dstMac,
    const IPv6Hdr& ipHdr,
    DHCPv6Packet& dhcpPacket) {
  /**
   * NOTE: relay forward packet handling is not tested thoroughly since we
   * don't have other relay agents running in the cluster;
   */
  // relay forward from other agent
  if (dhcpPacket.hopCount >= MAX_RELAY_HOPCOUNT) {
    XLOG(DBG2) << "Received DHCPv6 relay foward packet with max relay hopcount";
    sw->portStats(pkt->getSrcPort())->dhcpV6BadPkt();
    return;
  }
  // increment the hopcount and forward it
  dhcpPacket.hopCount++;
  auto vlan = pkt->getSrcVlanIf();
  auto serializeBody = [&](RWPrivateCursor* sendCursor) {
    dhcpPacket.write(sendCursor);
  };
  auto switchId = sw->getScopeResolver()->scope(pkt->getSrcPort()).switchId();
  MacAddress cpuMac = sw->getHwAsicTable()->getHwAsicIf(switchId)->getAsicMac();
  sendDHCPv6Packet(
      sw,
      dstMac,
      cpuMac,
      vlan,
      ipHdr.dstAddr,
      ipHdr.srcAddr,
      DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT,
      DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT,
      dhcpPacket.computePacketLength(),
      serializeBody);
}

void DHCPv6Handler::processDHCPv6RelayReply(
    SwSwitch* sw,
    std::unique_ptr<RxPacket> pkt,
    MacAddress /*srcMac*/,
    MacAddress /*dstMac*/,
    const IPv6Hdr& ipHdr,
    DHCPv6Packet& dhcpPacket) {
  auto state = sw->getState();

  auto switchIp = state->getDhcpV6ReplySrc();
  if (switchIp.isZero()) {
    switchIp = ipHdr.dstAddr;
  }
  auto intf = state->getInterfaces()->getInterface(RouterID(0), switchIp);
  if (!intf) {
    sw->portStats(pkt->getSrcPort())->dhcpV6DropPkt();
    XLOG(DBG2) << "Could not look up interface for " << switchIp
               << "DHCPv6 packet dropped";
    return;
  }

  // relay reply from the server
  MacAddress destMac;
  const uint8_t* relayData = nullptr;
  uint16_t relayLen = 0;
  std::unordered_set<uint16_t> selector = {
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID),
      static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_RELAY_MSG)};
  auto dhcpOptions = dhcpPacket.extractOptions(selector);
  for (int i = 0; i < dhcpOptions.size(); i++) {
    if (dhcpOptions[i].op ==
        static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_INTERFACE_ID)) {
      DCHECK(dhcpOptions[i].len == MacAddress::SIZE);
      destMac = MacAddress::fromBinary(
          folly::ByteRange(dhcpOptions[i].data, MacAddress::SIZE));
    } else if (
        dhcpOptions[i].op ==
        static_cast<uint16_t>(DHCPv6OptionType::DHCPv6_OPTION_RELAY_MSG)) {
      relayData = dhcpOptions[i].data;
      relayLen = dhcpOptions[i].len;
    }
  }
  if (destMac == MacAddress::ZERO || relayLen == 0) {
    sw->portStats(pkt->getSrcPort())->dhcpV6DropPkt();
    XLOG(DBG2) << "Bad dhcp relay reply message: malformed options";
    return;
  }
  /**
   * srcMac -> cpu mac, intf id -> dst mac
   * switch ip -> ip src, peerAddr -> ip dst,
   * relay message option -> send dhcp packet,
   */
  auto switchIds = sw->getScopeResolver()->scope(pkt->getSrcPort()).switchIds();
  CHECK_EQ(switchIds.size(), 1);
  MacAddress cpuMac =
      sw->getHwAsicTable()->getHwAsicIf(*switchIds.begin())->getAsicMac();
  // send dhcp packet
  auto serializeBody = [&](RWPrivateCursor* sendCursor) {
    sendCursor->push(relayData, relayLen);
  };
  sendDHCPv6Packet(
      sw,
      destMac,
      cpuMac,
      intf->getVlanID(),
      dhcpPacket.peerAddr,
      switchIp,
      DHCPv6Packet::DHCP6_CLIENT_UDPPORT,
      DHCPv6Packet::DHCP6_SERVERAGENT_UDPPORT,
      relayLen,
      serializeBody);
}

} // namespace facebook::fboss
