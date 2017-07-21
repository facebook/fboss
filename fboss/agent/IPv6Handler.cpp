/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/IPv6Handler.h"

#include <folly/MacAddress.h>
#include <folly/Format.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/DHCPv6Handler.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/packet/NDP.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/UDPHeader.h"

using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::unique_ptr;
using std::shared_ptr;

namespace facebook { namespace fboss {

template<typename BodyFn>
std::unique_ptr<TxPacket> createICMPv6Pkt(SwSwitch* sw,
                                          folly::MacAddress dstMac,
                                          folly::MacAddress srcMac,
                                          VlanID vlan,
                                          folly::IPAddressV6& dstIP,
                                          folly::IPAddressV6& srcIP,
                                          ICMPv6Type icmp6Type,
                                          ICMPv6Code icmp6Code,
                                          uint32_t bodyLength,
                                          BodyFn serializeBody) {
  IPv6Hdr ipv6(srcIP, dstIP);
  ipv6.trafficClass = 0xe0; // CS7 precedence (network control)
  ipv6.payloadLength = ICMPHdr::SIZE + bodyLength;
  ipv6.nextHeader = IP_PROTO_IPV6_ICMP;
  ipv6.hopLimit = 255;

  ICMPHdr icmp6(icmp6Type, icmp6Code, 0);

  uint32_t pktLen = icmp6.computeTotalLengthV6(bodyLength);
  auto pkt = sw->allocatePacket(pktLen);
  RWPrivateCursor cursor(pkt->buf());
  icmp6.serializeFullPacket(&cursor, dstMac, srcMac, vlan,
                               ipv6, bodyLength, serializeBody);
  return pkt;
}

struct IPv6Handler::ICMPHeaders {
  folly::MacAddress dst;
  folly::MacAddress src;
  const IPv6Hdr* ipv6;
  const ICMPHdr* icmp6;
};

IPv6Handler::IPv6Handler(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "IPv6Handler"),
      sw_(sw) {
}

void IPv6Handler::stateUpdated(const StateDelta& delta) {
  for (const auto& entry : delta.getIntfsDelta()) {
    if (!entry.getOld()) {
      intfAdded(delta.newState().get(), entry.getNew().get());
    } else if (!entry.getNew()) {
      intfDeleted(entry.getOld().get());
    } else {
      // TODO: We could add an intfChanged() method to re-use the existing
      // IPv6RouteAdvertiser object.
      intfDeleted(entry.getOld().get());
      intfAdded(delta.newState().get(), entry.getNew().get());
    }
  }
}

bool IPv6Handler::raEnabled(const Interface* intf) const {
  return intf->getNdpConfig().routerAdvertisementSeconds > 0;
}

void IPv6Handler::intfAdded(const SwitchState* state, const Interface* intf) {
  // If IPv6 router advertisement isn't enabled on this interface, ignore it.
  if (!raEnabled(intf)) {
    return;
  }

  IPv6RouteAdvertiser adv(sw_, state, intf);
  auto ret = routeAdvertisers_.emplace(intf->getID(), std::move(adv));
  CHECK(ret.second);
}

void IPv6Handler::intfDeleted(const Interface* intf) {
  if (!raEnabled(intf)) {
    return;
  }
  auto numErased = routeAdvertisers_.erase(intf->getID());
  CHECK_EQ(numErased, 1);
}

void IPv6Handler::handlePacket(unique_ptr<RxPacket> pkt,
                               MacAddress dst,
                               MacAddress src,
                               Cursor cursor) {
  const uint32_t l3Len = pkt->getLength() - (cursor - Cursor(pkt->buf()));
  IPv6Hdr ipv6(cursor);  // note: advances our cursor object
  VLOG(4) << "IPv6 (" << l3Len << " bytes)"
    " port: " << pkt->getSrcPort() <<
    " vlan: " << pkt->getSrcVlan() <<
    " src: " << ipv6.srcAddr.str() <<
    " (" << src << ")" <<
    " dst: " << ipv6.dstAddr.str() <<
    " (" << dst << ")" <<
    " nextHeader: " << static_cast<int>(ipv6.nextHeader);

  // retrieve the current switch state
  auto state = sw_->getState();
  PortID port = pkt->getSrcPort();

  // NOTE: DHCPv6 solicit packet from client has hoplimit set to 1,
  // we need to handle it before send the ICMPv6 TTL exceeded
  if (ipv6.nextHeader == IP_PROTO_UDP) {
    UDPHeader udpHdr;
    Cursor udpCursor(cursor);
    udpHdr.parse(sw_, port, &udpCursor);
    VLOG(4) << "DHCP UDP packet, source port :" << udpHdr.srcPort
        << " destination port: " << udpHdr.dstPort;
    if (DHCPv6Handler::isForDHCPv6RelayOrServer(udpHdr)) {
      DHCPv6Handler::handlePacket(sw_, std::move(pkt), src, dst, ipv6,
          udpHdr, udpCursor);
      return;
    }
  }

  // Get the Interface to which this packet should be forwarded in host
  // TODO:
  // 1. Assume VRF 0 now
  // 2. Only if v6 address has been assigned to an interface. For link local
  //    address that is supposed to be generated by default, we do not handle
  //    it now.
  std::shared_ptr<Interface> intf{nullptr};
  auto interfaceMap = state->getInterfaces();
  if (ipv6.dstAddr.isMulticast()) {
    // Forward multicast packet directly to corresponding host interface
    // and let Linux handle it. In software we consume ICMPv6 Multicast
    // packets for function of NDP protocol, rest all are forwarded to host.
    intf = interfaceMap->getInterfaceInVlanIf(pkt->getSrcVlan());
  } else if (ipv6.dstAddr.isLinkLocal()) {
    // Forward link-local packet directly to corresponding host interface
    // provided desAddr is assigned to that interface.
    intf = interfaceMap->getInterfaceInVlanIf(pkt->getSrcVlan());
    if (not intf->hasAddress(ipv6.dstAddr)) {
      intf = nullptr;
    }
  } else {
    // Else loopup host interface based on destAddr
    intf = interfaceMap->getInterfaceIf(RouterID(0), ipv6.dstAddr);
  }

  // If the packet is destined to us, accept packets
  // with a hop limit of 1. Else we need to forward
  // this packet so the hop limit should be at least 1
  auto minHopLimit = intf ? 0 : 1;
  if (ipv6.hopLimit <= minHopLimit) {
    VLOG(4) << "Rx IPv6 Packet with hop limit exceeded";
    sw_->stats()->port(port)->pktDropped();
    sw_->stats()->port(port)->ipv6HopExceeded();
    // Look up cpu mac from platform
    MacAddress cpuMac = sw_->getPlatform()->getLocalMac();
    sendICMPv6TimeExceeded(pkt->getSrcVlan(), cpuMac, cpuMac, ipv6, cursor);
    return;
  }

  // TODO: We need to clean up this code to do a proper job of determining
  // if the packet is for us or if it needs to be routed.
  // For now, we simply assume ICMPv6 packets are for us, and that other
  // packets need their MAC resolved.
  //
  // In the future, we need to fix this to look up the IP in our routing table.
  // If it's for us, or a multicast address we are subscribed to, we should
  // process it.  Otherwise, we need to route it.  If we don't have L2
  // forwarding info, then we need to resolve the next hop MAC.
  if (ipv6.nextHeader == IP_PROTO_IPV6_ICMP) {
    pkt = handleICMPv6Packet(std::move(pkt), dst, src, ipv6, cursor);
    if (pkt == nullptr) {
      // packet has been handled
      return;
    }
  }

  if (intf) {
    // packets destined for us
    // Anything not handled by the controller, we will forward it to the host,
    // i.e. ping, ssh, bgp...
    PortID portID = pkt->getSrcPort();
    if (sw_->sendPacketToHost(intf->getID(), std::move(pkt))) {
      sw_->stats()->port(portID)->pktToHost(l3Len);
    } else {
      sw_->stats()->port(portID)->pktDropped();
    }
    return;
  }

  // For now, assume we need to resolve the IP for this packet.
  // TODO: Add rate limiting so we don't generate too many requests for the
  // same IP.  Following the rules in RFC 4861 should be sufficient.
  sendNeighborSolicitations(pkt->getSrcPort(), ipv6.dstAddr);
  // We drop the packet while waiting on a response.
  sw_->portStats(pkt)->pktDropped();
}

unique_ptr<RxPacket> IPv6Handler::handleICMPv6Packet(
    unique_ptr<RxPacket> pkt,
    MacAddress dst,
    MacAddress src,
    const IPv6Hdr& ipv6,
    Cursor cursor) {
  ICMPHdr icmp6(cursor); // note: advances our cursor object

  // Validate the checksum, and drop the packet if it is not valid
  if (!icmp6.validateChecksum(ipv6, cursor)) {
    VLOG(3) << "bad ICMPv6 checksum";
    sw_->portStats(pkt)->pktDropped();
    return nullptr;
  }

  ICMPHeaders hdr{dst, src, &ipv6, &icmp6};
  switch (icmp6.type) {
    case ICMPV6_TYPE_NDP_ROUTER_SOLICITATION:
      handleRouterSolicitation(std::move(pkt), hdr, cursor);
      return nullptr;
    case ICMPV6_TYPE_NDP_ROUTER_ADVERTISEMENT:
      handleRouterAdvertisement(std::move(pkt), hdr, cursor);
      return nullptr;
    case ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION:
      handleNeighborSolicitation(std::move(pkt), hdr, cursor);
      return nullptr;
    case ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT:
      handleNeighborAdvertisement(std::move(pkt), hdr, cursor);
      return nullptr;
    case ICMPV6_TYPE_NDP_REDIRECT_MESSAGE:
      sw_->portStats(pkt)->ipv6NdpPkt();
      // TODO: Do we need to bother handling this yet?
      sw_->portStats(pkt)->pktDropped();
      return nullptr;
    default:
      break;
  }

  return pkt;
}

void IPv6Handler::handleRouterSolicitation(unique_ptr<RxPacket> pkt,
                                           const ICMPHeaders& hdr,
                                           Cursor cursor) {
  sw_->portStats(pkt)->ipv6NdpPkt();
  if (!checkNdpPacket(hdr, pkt.get())) {
    return;
  }

  cursor.skip(4); // 4 reserved bytes

  auto state = sw_->getState();
  auto vlan = state->getVlans()->getVlanIf(pkt->getSrcVlan());
  if (!vlan) {
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  auto intf = state->getInterfaces()->getInterfaceIf(vlan->getInterfaceID());
  if (!intf) {
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  MacAddress dstMac = hdr.src;
  while (cursor.totalLength() != 0) {
    auto optionType = cursor.read<uint8_t>();
    auto optionLength = cursor.read<uint8_t>();
    if (optionType == NDPOptionType::SRC_LL_ADDRESS) {
      // target mac address
      if (optionLength != NDPOptionLength::SRC_LL_ADDRESS_IEEE802) {
        VLOG(3) << "bad option length " <<
          static_cast<unsigned int>(optionLength) <<
          " for source MAC address in IPv6 router solicitation";
        sw_->portStats(pkt)->pktDropped();
        return;
      }
      dstMac = PktUtil::readMac(&cursor);
    } else {
      // Unknown option.  Just skip over it.
      cursor.skip(optionLength * 8);
    }
  }

  // Send the response
  IPAddressV6 dstIP = hdr.ipv6->srcAddr;
  if (dstIP.isZero()) {
    dstIP = IPAddressV6("ff01::1");
  }

  VLOG(3) << "sending router advertisement in response to solicitation from "
    << dstIP.str() << " (" << dstMac << ")";

  uint32_t pktLen = IPv6RouteAdvertiser::getPacketSize(intf.get());
  auto resp = sw_->allocatePacket(pktLen);
  RWPrivateCursor respCursor(resp->buf());
  IPv6RouteAdvertiser::createAdvertisementPacket(
    intf.get(), &respCursor, dstMac, dstIP);
  sw_->sendPacketSwitched(std::move(resp));
}

void IPv6Handler::handleRouterAdvertisement(
    unique_ptr<RxPacket> pkt,
    const ICMPHeaders& hdr,
    Cursor /*cursor*/) {
  sw_->portStats(pkt)->ipv6NdpPkt();
  if (!checkNdpPacket(hdr, pkt.get())) {
    return;
  }

  if (!hdr.ipv6->srcAddr.isLinkLocal()) {
    VLOG(6) << "bad IPv6 router advertisement: source address must be "
      "link-local: " << hdr.ipv6->srcAddr;
    sw_->portStats(pkt)->ipv6NdpBad();
    return;
  }

  VLOG(3) << "dropping IPv6 router advertisement from " << hdr.ipv6->srcAddr;
  sw_->portStats(pkt)->pktDropped();
}

void IPv6Handler::handleNeighborSolicitation(unique_ptr<RxPacket> pkt,
                                             const ICMPHeaders& hdr,
                                             Cursor cursor) {
  sw_->portStats(pkt)->ipv6NdpPkt();
  if (!checkNdpPacket(hdr, pkt.get())) {
    return;
  }

  cursor.skip(4); // 4 reserved bytes
  IPAddressV6 targetIP = PktUtil::readIPv6(&cursor);
  if (targetIP.isMulticast()) {
    VLOG(6) << "bad IPv6 neighbor solicitation request: target is "
      "multicast: " << targetIP;
    sw_->portStats(pkt)->ipv6NdpBad();
    return;
  }
  VLOG(4) << "got neighbor solicitation for " << targetIP.str();

  auto state = sw_->getState();
  auto vlan = state->getVlans()->getVlanIf(pkt->getSrcVlan());
  if (!vlan) {
    // Hmm, we don't actually have this VLAN configured.
    // Perhaps the state has changed since we received the packet.
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  auto updater = sw_->getNeighborUpdater();
  auto type = ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION;

  // Check to see if this IP address is in our NDP response table.
  auto entry = vlan->getNdpResponseTable()->getEntry(targetIP);
  if (!entry) {
    updater->receivedNdpNotMine(vlan->getID(), hdr.ipv6->srcAddr, hdr.src,
                                PortDescriptor::fromRxPacket(*pkt.get()),
                                type, 0);
    return;
  }

  updater->receivedNdpMine(vlan->getID(), hdr.ipv6->srcAddr, hdr.src,
                           PortDescriptor::fromRxPacket(*pkt.get()),
                           type, 0);

  // TODO: It might be nice to support duplicate address detection, and track
  // whether our IP is tentative or not.

  // Send the response
  sendNeighborAdvertisement(pkt->getSrcVlan(),
                            entry.value().mac, targetIP,
                            hdr.src, hdr.ipv6->srcAddr);
}

void IPv6Handler::handleNeighborAdvertisement(unique_ptr<RxPacket> pkt,
                                              const ICMPHeaders& hdr,
                                              Cursor cursor) {
  sw_->portStats(pkt)->ipv6NdpPkt();
  if (!checkNdpPacket(hdr, pkt.get())) {
    return;
  }

  auto flags = cursor.read<uint32_t>();
  IPAddressV6 targetIP = PktUtil::readIPv6(&cursor);

  // Check for options fields.  The target MAC address may be specified here.
  // If it isn't, we use the source MAC from the ethernet header.
  MacAddress targetMac = hdr.src;
  while (cursor.totalLength() != 0) {
    auto optionType = cursor.read<uint8_t>();
    auto optionLength = cursor.read<uint8_t>();
    if (optionType == NDPOptionType::TARGET_LL_ADDRESS) {
      // target mac address
      if (optionLength != NDPOptionLength::TARGET_LL_ADDRESS_IEEE802) {
        VLOG(3) << "bad option length " <<
          static_cast<unsigned int>(optionLength) <<
          " for target MAC address in IPv6 neighbor advertisement";
        sw_->portStats(pkt)->pktDropped();
        return;
      }
      targetMac = PktUtil::readMac(&cursor);
    } else {
      // Unknown option.  Just skip over it.
      cursor.skip(optionLength * 8);
    }
  }

  if (targetMac.isMulticast() || targetMac.isBroadcast()) {
    VLOG(3) << "ignoring IPv6 neighbor advertisement for " << targetIP <<
      "with multicast MAC " << targetMac;
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  auto state = sw_->getState();
  auto vlan = state->getVlans()->getVlanIf(pkt->getSrcVlan());
  if (!vlan) {
    // Hmm, we don't actually have this VLAN configured.
    // Perhaps the state has changed since we received the packet.
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  VLOG(4) << "got neighbor advertisement for " << targetIP <<
    " (" << targetMac << ")";

  auto updater = sw_->getNeighborUpdater();
  auto type = ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT;

  // Check to see if this IP address is in our NDP response table.
  auto entry = vlan->getNdpResponseTable()->getEntry(hdr.ipv6->dstAddr);
  if (!entry) {
    updater->receivedNdpNotMine(vlan->getID(), targetIP, hdr.src,
                                PortDescriptor::fromRxPacket(*pkt.get()),
                                type, flags);
    return;
  }

  updater->receivedNdpMine(vlan->getID(), targetIP, hdr.src,
                           PortDescriptor::fromRxPacket(*pkt.get()),
                           type, flags);
}


void IPv6Handler::sendICMPv6TimeExceeded(VlanID srcVlan,
                              MacAddress dst,
                              MacAddress src,
                              IPv6Hdr& v6Hdr,
                              folly::io::Cursor cursor) {
  auto state = sw_->getState();

  // payload serialization function
  // 4 bytes unused + ipv6 header + as much payload as possible to fit MTU
  // this is upper limit of bodyLength
  uint32_t bodyLengthLimit = IPV6_MIN_MTU - ICMPHdr::computeTotalLengthV6(0);
  // this is when we add the whole input L3 packet
  uint32_t fullPacketLength = ICMPHdr::ICMPV6_UNUSED_LEN + IPv6Hdr::SIZE
                              + cursor.totalLength();
  auto bodyLength = std::min(bodyLengthLimit, fullPacketLength);
  auto serializeBody = [&](RWPrivateCursor* sendCursor) {
    sendCursor->writeBE<uint32_t>(0);
    v6Hdr.serialize(sendCursor);
    auto remainingLength = bodyLength - IPv6Hdr::SIZE -
                           ICMPHdr::ICMPV6_UNUSED_LEN;
    sendCursor->push(cursor, remainingLength);
  };

  IPAddressV6 srcIp = getSwitchVlanIPv6(state, srcVlan);
  auto icmpPkt = createICMPv6Pkt(sw_, dst, src, srcVlan,
                             v6Hdr.srcAddr, srcIp,
                             ICMPV6_TYPE_TIME_EXCEEDED,
                             ICMPV6_CODE_TIME_EXCEEDED_HOPLIMIT_EXCEEDED,
                             bodyLength, serializeBody);
  VLOG(4) << "sending ICMPv6 Time Exceeded with srcMac  " << src
          << " dstMac: " << dst
          << " vlan: " << srcVlan
          << " dstIp: " << v6Hdr.srcAddr.str()
          << " srcIP: " << srcIp.str()
          << " bodyLength: " << bodyLength;
  sw_->sendPacketSwitched(std::move(icmpPkt));
}

bool IPv6Handler::checkNdpPacket(const ICMPHeaders& hdr,
                                 const RxPacket* pkt) const {
  // Validation common for all NDP packets
  if (hdr.ipv6->hopLimit != 255) {
    VLOG(3) << "bad IPv6 NDP request (" << hdr.icmp6->type <<
      "): hop limit should be 255, received value is " <<
      static_cast<int>(hdr.ipv6->hopLimit);
    sw_->portStats(pkt)->ipv6NdpBad();
    return false;
  }
  if (hdr.icmp6->code != 0) {
    VLOG(3) << "bad IPv6 NDP request (" << hdr.icmp6->type <<
      "): code should be 0, received value is " << hdr.icmp6->code;
    sw_->portStats(pkt)->ipv6NdpBad();
    return false;
  }

  return true;
}

void IPv6Handler::sendNeighborSolicitation(SwSwitch* sw,
                                           const IPAddressV6& targetIP,
                                           const MacAddress& srcMac,
                                           const VlanID vlanID) {
  uint32_t bodyLength = 4 + 16 + 8;

  auto serializeBody = [&](RWPrivateCursor* cursor) {
    cursor->writeBE<uint32_t>(0); // reserved
    cursor->push(targetIP.bytes(), 16);
    cursor->write<uint8_t>(1); // Source link layer address option
    cursor->write<uint8_t>(1); // Option l5ength = 1 (x8)
    cursor->push(srcMac.bytes(), MacAddress::SIZE);
  };

  IPAddressV6 solicitedNodeAddr = targetIP.getSolicitedNodeAddress();
  MacAddress dstMac = MacAddress::createMulticast(solicitedNodeAddr);
  // For now, we always use our link local IP as the source.
  IPAddressV6 srcIP(IPAddressV6::LINK_LOCAL, srcMac);
  auto pkt = createICMPv6Pkt(sw, dstMac, srcMac, vlanID,
                             solicitedNodeAddr, srcIP,
                             ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
                             ICMPV6_CODE_NDP_MESSAGE_CODE,
                             bodyLength, serializeBody);

  VLOG(4) << "sending neighbor solicitation for " << targetIP <<
    " on vlan " << vlanID;
  sw->sendPacketSwitched(std::move(pkt));
}

void IPv6Handler::sendNeighborSolicitation(SwSwitch* sw,
                                           const IPAddressV6& targetIP,
                                           const shared_ptr<Vlan>& vlan) {
  auto state = sw->getState();
  auto intfID = vlan->getInterfaceID();

  if (!Interface::isIpAttached(targetIP, intfID, state)) {
    VLOG(0) << "Cannot reach " << targetIP << " on interface " << intfID;
    return;
  }

  auto intf = state->getInterfaces()->getInterfaceIf(intfID);
  if (!intf) {
    VLOG(0) << "Cannot find interface " << intfID;
    return;
  }

  sendNeighborSolicitation(sw, targetIP, intf->getMac(), vlan->getID());
}

void IPv6Handler::sendNeighborSolicitations(
    PortID ingressPort, const folly::IPAddressV6& targetIP) {
  // Don't send solicitations for multicast or broadcast addresses.
  if (targetIP.isMulticast() || targetIP.isLinkLocalBroadcast()) {
    return;
  }

  auto state = sw_->getState();

  // TODO: assume vrf 0 now
  auto routeTable = state->getRouteTables()->getRouteTableIf(RouterID(0));
  if (!routeTable) {
    return;
  }

  auto route = routeTable->getRibV6()->longestMatch(targetIP);
  if (!route || !route->isResolved()) {
    sw_->stats()->port(ingressPort)->ipv6DstLookupFailure();
    // No way to reach targetIP
    return;
  }

  auto intfs = state->getInterfaces();
  auto nhs = route->getForwardInfo().getNextHopSet();
  for (auto nh : nhs) {
    auto intf = intfs->getInterfaceIf(nh.intf());
    if (intf) {
      auto source = intf->getAddressToReach(nh.addr())->first.asV6();
      auto target = route->isConnected() ? targetIP : nh.addr().asV6();
      if (source == target) {
        // This packet is for us.  Don't send NDP requests to ourself.
        continue;
      }

      auto vlanID = intf->getVlanID();
      auto vlan = state->getVlans()->getVlanIf(vlanID);
      if (vlan) {
        auto entry = vlan->getNdpTable()->getEntryIf(target);
        if (entry == nullptr) {
          // No entry in NDP table, create a neighbor solicitation packet
          sendNeighborSolicitation(sw_, target, intf->getMac(), vlan->getID());

          // Notify the updater that we sent a solicitation out
          sw_->getNeighborUpdater()->sentNeighborSolicitation(vlanID, target);
        } else {
          VLOG(5) << "not sending neighbor solicitation for " << target.str()
                  << ", " << ((entry->isPending()) ? "pending" : "")
                  << " entry already exists";

        }
      }
    }
  }
}

void IPv6Handler::floodNeighborAdvertisements() {
  for (const auto& intf: *sw_->getState()->getInterfaces()) {
    for (const auto& addrEntry: intf->getAddresses()) {
      if (!addrEntry.first.isV6()) {
        continue;
      }
      sendNeighborAdvertisement(intf->getVlanID(), intf->getMac(),
          addrEntry.first.asV6(), MacAddress::BROADCAST, IPAddressV6());
    }
  }
}

void IPv6Handler::sendNeighborAdvertisement(VlanID vlan,
                                            MacAddress srcMac,
                                            IPAddressV6 srcIP,
                                            MacAddress dstMac,
                                            IPAddressV6 dstIP) {
  VLOG(3) << "sending neighbor advertisement to " << dstIP.str()
    << " (" << dstMac << "): for " <<  srcIP << " (" << srcMac << ")";

  uint32_t flags = 0xa0000000; // router, override
  if (dstIP.isZero()) {
    // TODO: add a constructor that doesn't require string processing
    dstIP = IPAddressV6("ff01::1");
  } else {
    // Set the solicited flag
    flags |= 0x40000000;
  }

  uint32_t bodyLength = 4 + 16 + 8;
  auto serializeBody = [&](RWPrivateCursor* cursor) {
    cursor->writeBE<uint32_t>(flags);
    cursor->push(srcIP.bytes(), IPAddressV6::byteCount());
    cursor->write<uint8_t>(NDPOptionType::TARGET_LL_ADDRESS);
    cursor->write<uint8_t>(NDPOptionLength::TARGET_LL_ADDRESS_IEEE802);
    cursor->push(srcMac.bytes(), MacAddress::SIZE);
  };

  auto pkt = createICMPv6Pkt(sw_, dstMac, srcMac, vlan, dstIP, srcIP,
                             ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
                             ICMPV6_CODE_NDP_MESSAGE_CODE,
                             bodyLength, serializeBody);
  sw_->sendPacketSwitched(std::move(pkt));
}

}} // facebook::fboss
