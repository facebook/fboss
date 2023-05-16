/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/IPv4Handler.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/DHCPv4Handler.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/IPHeaderV4.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Route.h"

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

using folly::IPAddress;
using folly::IPAddressV4;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::unique_ptr;

namespace facebook::fboss {

template <typename BodyFn>
std::unique_ptr<TxPacket> createICMPv4Pkt(
    SwSwitch* sw,
    folly::MacAddress dstMac,
    folly::MacAddress srcMac,
    std::optional<VlanID> vlan,
    folly::IPAddressV4& dstIP,
    folly::IPAddressV4& srcIP,
    ICMPv4Type icmpType,
    ICMPv4Code icmpCode,
    uint32_t bodyLength,
    BodyFn serializeBody) {
  IPv4Hdr ipv4(
      srcIP,
      dstIP,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_ICMP),
      ICMPHdr::SIZE + bodyLength);
  ipv4.computeChecksum();

  ICMPHdr icmp4(
      static_cast<uint8_t>(icmpType), static_cast<uint8_t>(icmpCode), 0);
  uint32_t pktLen = icmp4.computeTotalLengthV4(bodyLength, vlan.has_value());

  auto pkt = sw->allocatePacket(pktLen);
  RWPrivateCursor cursor(pkt->buf());
  icmp4.serializeFullPacket(
      &cursor, dstMac, srcMac, vlan, ipv4, bodyLength, serializeBody);
  return pkt;
}

IPv4Handler::IPv4Handler(SwSwitch* sw) : sw_(sw) {}

void IPv4Handler::sendICMPTimeExceeded(
    PortID port,
    std::optional<VlanID> srcVlan,
    MacAddress dst,
    MacAddress src,
    IPv4Hdr& v4Hdr,
    Cursor cursor) {
  auto state = sw_->getState();

  // payload serialization function
  // 4 bytes unused + ipv4 header + 8 bytes payload
  auto bodyLength =
      ICMPHdr::ICMPV4_UNUSED_LEN + v4Hdr.size() + ICMPHdr::ICMPV4_SENDER_BYTES;
  auto serializeBody = [&](RWPrivateCursor* sendCursor) {
    sendCursor->writeBE<uint32_t>(0); // unused bytes
    v4Hdr.write(sendCursor);
    sendCursor->push(cursor.data(), ICMPHdr::ICMPV4_SENDER_BYTES);
  };

  IPAddressV4 srcIp;
  try {
    srcIp =
        getSwitchIntfIP(state, sw_->getState()->getInterfaceIDForPort(port));
  } catch (const std::exception& ex) {
    srcIp = getAnyIntfIP(state);
  }

  auto icmpPkt = createICMPv4Pkt(
      sw_,
      dst,
      src,
      srcVlan,
      v4Hdr.srcAddr,
      srcIp,
      ICMPv4Type::ICMPV4_TYPE_TIME_EXCEEDED,
      ICMPv4Code::ICMPV4_CODE_TIME_EXCEEDED_TTL_EXCEEDED,
      bodyLength,
      serializeBody);

  auto srcVlanStr = srcVlan.has_value()
      ? folly::to<std::string>(static_cast<int>(srcVlan.value()))
      : "None";
  XLOG(DBG4) << "sending ICMP Time Exceeded with srcMac " << src
             << " dstMac: " << dst << " vlan: " << srcVlanStr
             << " dstIp: " << v4Hdr.srcAddr.str() << " srcIp: " << srcIp.str()
             << " bodyLength: " << bodyLength;
  sw_->sendPacketSwitchedAsync(std::move(icmpPkt));
}

void IPv4Handler::handlePacket(
    unique_ptr<RxPacket> pkt,
    MacAddress dst,
    MacAddress src,
    Cursor cursor) {
  SwitchStats* stats = sw_->stats();
  PortID port = pkt->getSrcPort();
  auto vlanID = pkt->getSrcVlanIf();

  const uint32_t l3Len = pkt->getLength() - (cursor - Cursor(pkt->buf()));
  stats->port(port)->ipv4Rx();
  IPv4Hdr v4Hdr(cursor);
  XLOG(DBG4) << "Rx IPv4 packet (" << l3Len << " bytes) " << v4Hdr.srcAddr.str()
             << " --> " << v4Hdr.dstAddr.str() << " proto: 0x" << std::hex
             << static_cast<int>(v4Hdr.protocol);

  // Additional data (such as FCS) may be appended after the IP payload
  auto payload =
      folly::IOBuf::wrapBuffer(cursor.data(), v4Hdr.length - v4Hdr.size());
  cursor.reset(payload.get());

  // retrieve the current switch state
  auto state = sw_->getState();
  // Need to check if the packet is for self or not. We store our IP
  // in the ARP response table. Use that for now.
  auto portPtr = state->getPorts()->getPortIf(port);
  if (!portPtr) {
    // Received packed on unknown port
    stats->port(port)->pktDropped();
    return;
  }
  auto intfID = sw_->getState()->getInterfaceIDForPort(port);
  auto ingressInterface = state->getInterfaces()->getNodeIf(intfID);
  if (!ingressInterface) {
    // Received packed on unknown port / interface
    stats->port(port)->pktDropped();
    return;
  }

  if (v4Hdr.protocol == static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
    Cursor udpCursor(cursor);
    UDPHeader udpHdr;
    udpHdr.parse(&udpCursor, sw_->portStats(port));
    XLOG(DBG4) << "UDP packet, Source port :" << udpHdr.srcPort
               << " destination port: " << udpHdr.dstPort;
    if (DHCPv4Handler::isDHCPv4Packet(udpHdr)) {
      DHCPv4Handler::handlePacket(
          sw_, std::move(pkt), src, dst, v4Hdr, udpHdr, udpCursor);
      return;
    }
  }

  // Handle packets destined for us
  // Get the Interface to which this packet should be forwarded in host
  // TODO: assume vrf 0 now
  std::shared_ptr<Interface> intf{nullptr};
  auto interfaceMap = state->getInterfaces();
  if (v4Hdr.dstAddr.isMulticast()) {
    // If packet is received on a lag member port, ensure that
    // LAG is in forwarding state
    if (!AggregatePort::isIngressValid(state, pkt)) {
      XLOG_EVERY_MS(DBG2, 5000)
          << "Dropping multicast ipv4 pkt ingressing on disabled agg member port "
          << pkt->getSrcPort();
      return;
    }
    // Forward multicast packet directly to corresponding host interface
    auto intfID = sw_->getState()->getInterfaceIDForPort(port);
    intf = state->getInterfaces()->getNodeIf(intfID);
  } else if (v4Hdr.dstAddr.isLinkLocal()) {
    // XXX: Ideally we should scope the limit to Link only. However we are
    // using v4 link locals in a special way on Galaxy/6pack which needs because
    // of which we do not limit the scope.
    //
    // Forward link-local packet directly to corresponding host interface
    // provided desAddr is assigned to that interface.
    // auto intfID = sw_->getState()->getInterfaceIDForPort(port);
    // intf = state->getInterfaces()->getNodeIf(intfID);
    // if (not intf->hasAddress(v4Hdr.dstAddr)) {
    //   intf = nullptr;
    // }
    intf = interfaceMap->getInterfaceIf(RouterID(0), v4Hdr.dstAddr);
  } else {
    // Else loopup host interface based on destAddr
    intf = interfaceMap->getInterfaceIf(RouterID(0), v4Hdr.dstAddr);
  }

  if (intf) {
    // TODO: Also check to see if this is the broadcast address for one of the
    // interfaces on this VLAN.  We should probably build up a more efficient
    // data structure to look up this information.
    stats->port(port)->ipv4Mine();
    // Anything not handled by the controller, we will forward it to the host,
    // i.e. ping, ssh, bgp...
    // FixME: will do another diff to set length in RxPacket, so that it
    // can be reused here.
    if (sw_->sendPacketToHost(intf->getID(), std::move(pkt))) {
      stats->port(port)->pktToHost(l3Len);
    } else {
      stats->port(port)->pktDropped();
    }
    return;
  }

  // if packet is not for us, check the ttl exceed
  if (v4Hdr.ttl <= 1) {
    XLOG(DBG4) << "Rx IPv4 Packet with TTL expired";
    stats->port(port)->pktDropped();
    stats->port(port)->ipv4TtlExceeded();
    // Look up cpu mac from switchInfo
    auto switchId = sw_->getScopeResolver()->scope(port).switchId();
    MacAddress cpuMac =
        sw_->getHwAsicTable()->getHwAsicIf(switchId)->getAsicMac();
    sendICMPTimeExceeded(port, vlanID, cpuMac, cpuMac, v4Hdr, cursor);
    return;
  }

  // Handle broadcast packets.
  // TODO: Also check to see if this is the broadcast address for one of the
  // interfaces on this VLAN. We should probably build up a more efficient
  // data structure to look up this information.
  if (v4Hdr.dstAddr.isLinkLocalBroadcast()) {
    stats->port(port)->pktDropped();
    return;
  }

  // TODO: check the reason of punt, for now, assume it is for
  // resolving the address
  // We will need to manage the rate somehow. Either from HW
  // or a SW control here
  stats->port(port)->ipv4Nexthop();
  if (!resolveMac(state, port, v4Hdr.dstAddr)) {
    stats->port(port)->ipv4NoArp();
    XLOG(DBG4) << "Cannot find the interface to send out ARP request for "
               << v4Hdr.dstAddr.str();
  }
  // TODO: ideally, we need to store this packet until the ARP is done and
  // then send this pkt out. For now, just drop it.
  stats->port(port)->pktDropped();
}

// Return true if we successfully sent an ARP request, false otherwise
bool IPv4Handler::resolveMac(
    std::shared_ptr<SwitchState> state,
    PortID ingressPort,
    IPAddressV4 dest) {
  // need to find out our own IP and MAC addresses so that we can send the
  // ARP request out. Since the request will be broadcast, there is no need to
  // worry about which port to send the packet out.
  auto route = sw_->longestMatch(state, dest, RouterID(0));

  if (!route || !route->isResolved()) {
    sw_->portStats(ingressPort)->ipv4DstLookupFailure();
    // No way to reach dest
    return false;
  }

  auto intfs = state->getInterfaces();
  auto nhs = route->getForwardInfo().getNextHopSet();
  auto sent = false;
  for (auto nh : nhs) {
    auto intf = intfs->getNodeIf(nh.intf());
    if (intf) {
      if (nh.addr().isV4()) {
        auto source = intf->getAddressToReach(nh.addr())->first.asV4();
        auto target = route->isConnected() ? dest : nh.addr().asV4();
        if (source == target) {
          // This packet is for us.  Don't send ARP requess for our own IP.
          continue;
        }

        sent = sw_->sendArpRequestHelper(intf, state, source, target);

      } else if (nh.addr().isV6() && !route->isConnected()) {
        auto source = intf->getAddressToReach(nh.addr())->first.asV6();
        auto target = nh.addr().asV6();
        if (source == target) {
          // This packet is for us.  Don't send NDP requests to ourself.
          continue;
        }

        sent = sw_->sendNdpSolicitationHelper(intf, state, target);
      }
    }
  }

  return sent;
}

} // namespace facebook::fboss
