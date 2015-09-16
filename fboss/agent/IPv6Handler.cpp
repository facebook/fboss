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

  // NOTE: DHCPv6 solicit pacekt from client has hoplimit set to 1,
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

  if (ipv6.hopLimit <= 1) {
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

  // TODO:
  // 1. Assume VRF 0 now
  // 2. Only if v6 address has been assigned to an interface. For link local
  //    address that is supposed to be generated by default, we do not handle
  //    it now.
  auto intf = state->getInterfaces()->getInterfaceIf(
      RouterID(0), folly::IPAddress(ipv6.dstAddr));
  if (intf) {
    // packets destined for us
    // Anything not handled by the controller, we will forward it to the host,
    // i.e. ping, ssh, bgp...
    PortID port = pkt->getSrcPort();
    if (sw_->sendPacketToHost(std::move(pkt))) {
      sw_->stats()->port(port)->pktToHost(l3Len);
    } else {
      sw_->stats()->port(port)->pktDropped();
    }
    return;
  }

  // For now, assume we need to resolve the IP for this packet.
  // TODO: Add rate limiting so we don't generate too many requests for the
  // same IP.  Following the rules in RFC 4861 should be sufficient.
  sendNeighborSolicitations(ipv6.dstAddr);
  // We drop the packet while waiting on a response.
  sw_->portStats(pkt)->pktDropped();
}

uint32_t IPv6Handler::flushNdpEntryBlocking(IPAddressV6 ip, VlanID vlan) {
  uint32_t count{0};
  auto updateFn = [&](const shared_ptr<SwitchState>& state) {
    return performNeighborFlush(state, vlan, ip, &count);
  };
  sw_->updateStateBlocking("flush NDP entry", updateFn);
  return count;
}

shared_ptr<SwitchState> IPv6Handler::performNeighborFlush(
    const shared_ptr<SwitchState>& state,
    VlanID vlanID,
    folly::IPAddressV6 ip,
    uint32_t* countFlushed) {
  *countFlushed = 0;
  shared_ptr<SwitchState> newState{state};
  if (vlanID == VlanID(0)) {
    // Flush this IP from every VLAN that contains an entry for it.
    for (const auto& vlan : *newState->getVlans()) {
      if (performNeighborFlush(&newState, vlan.get(), ip)) {
        ++(*countFlushed);
      }
    }
  } else {
    auto* vlan = state->getVlans()->getVlan(vlanID).get();
    if (performNeighborFlush(&newState, vlan, ip)) {
      ++(*countFlushed);
    }
  }
  return newState;
}

bool IPv6Handler::performNeighborFlush(shared_ptr<SwitchState>* state,
                                      Vlan* vlan,
                                      folly::IPAddressV6 ip) {
  auto* ndpTable = vlan->getNdpTable().get();
  const auto& entry = ndpTable->getNodeIf(ip);
  if (!entry) {
    return false;
  }

  ndpTable = ndpTable->modify(&vlan, state);
  ndpTable->removeNode(ip);
  return true;
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

  return std::move(pkt);
}

void IPv6Handler::handleRouterSolicitation(unique_ptr<RxPacket> pkt,
                                           const ICMPHeaders& hdr,
                                           Cursor cursor) {
  sw_->portStats(pkt)->ipv6NdpPkt();
  if (!checkNdpPacket(hdr, pkt.get())) {
    return;
  }

  // TODO: process the packet
  sw_->portStats(pkt)->pktDropped();
}

void IPv6Handler::handleRouterAdvertisement(unique_ptr<RxPacket> pkt,
                                            const ICMPHeaders& hdr,
                                            Cursor cursor) {
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

  // Check to see if this IP address is in our NDP response table.
  auto entry = vlan->getNdpResponseTable()->getEntry(targetIP);
  if (!entry) {
    // The target IP does not refer to us.
    VLOG(4) << "ignoring neighbor solicitation for " << targetIP.str();
    sw_->portStats(pkt)->pktDropped();
    // Note that ARP updates the forward entry mapping here if necessary.
    // We could potentially do the same here, although the IPv6 NDP RFC
    // doesn't appear to recommend this--it states that we MUST silently
    // discard the packet if the target IP isn't ours.
    return;
  }

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

  auto flags = cursor.readBE<uint32_t>();
  IPAddressV6 targetIP = PktUtil::readIPv6(&cursor);

  // Check for options fields.  The target MAC address may be specified here.
  // If it isn't, we use the source MAC from the ethernet header.
  MacAddress targetMac = hdr.src;
  if (cursor.totalLength() != 0) {
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

  VLOG(4) << "got neighbor advertisement for " << targetIP <<
    " (" << targetMac << ")";

  updateNeighborEntry(pkt.get(), targetIP, targetMac, flags);
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

void IPv6Handler::sendNeighborSolicitation(
    const folly::IPAddressV6& targetIP,
    const shared_ptr<Interface> intf,
    const shared_ptr<Vlan> vlan) {
  uint32_t bodyLength = 4 + 16 + 8;
  auto serializeBody = [&](RWPrivateCursor* cursor) {
    cursor->writeBE<uint32_t>(0); // reserved
    cursor->push(targetIP.bytes(), 16);
    cursor->write<uint8_t>(1); // Source link layer address option
    cursor->write<uint8_t>(1); // Option length = 1 (x8)
    cursor->push(intf->getMac().bytes(), MacAddress::SIZE);
  };

  IPAddressV6 solicitedNodeAddr = targetIP.getSolicitedNodeAddress();
  MacAddress dstMac = MacAddress::createMulticast(solicitedNodeAddr);
  // For now, we always use our link local IP as the source.
  IPAddressV6 srcIP(IPAddressV6::LINK_LOCAL, intf->getMac());
  auto pkt = createICMPv6Pkt(sw_, dstMac, intf->getMac(), intf->getVlanID(),
                             solicitedNodeAddr, srcIP,
                             ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
                             ICMPV6_CODE_NDP_MESSAGE_CODE,
                             bodyLength, serializeBody);
  VLOG(4) << "adding pending NDP entry for " << targetIP;
  setPendingNdpEntry(intf->getID(), vlan, targetIP);


  VLOG(4) << "sending neighbor solicitation for " << targetIP <<
    " on interface " << intf->getID();
  sw_->sendPacketSwitched(std::move(pkt));
}

void IPv6Handler::sendNeighborSolicitations(
    const folly::IPAddressV6& targetIP) {
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
    // No way to reach targetIP
    return;
  }

  auto intfs = state->getInterfaces();
  auto nhs = route->getForwardInfo().getNexthops();
  for (auto nh : nhs) {
    auto intf = intfs->getInterfaceIf(nh.intf);
    if (intf) {
      auto source = intf->getAddressToReach(nh.nexthop)->first.asV6();
      auto target = route->isConnected() ? targetIP : nh.nexthop.asV6();
      if (source == target) {
        // This packet is for us.  Don't send NDP requests to ourself.
        // TODO(aeckert): #5478027 make sure we don't solicit any local address.
        continue;
      }

      auto vlanID = intf->getVlanID();
      auto vlan = state->getVlans()->getVlanIf(vlanID);
      if (vlan) {
        auto entry = vlan->getNdpTable()->getEntryIf(target);
        if (entry == nullptr) {
          // No entry in NDP table, create a neighbor solicitation packet
          sendNeighborSolicitation(target, intf, vlan);
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

void IPv6Handler::setPendingNdpEntry(InterfaceID intfID,
                                     shared_ptr<Vlan> vlan,
                                     const folly::IPAddressV6 &ip) {
  auto vlanID = vlan->getID();
  VLOG(4) << "setting pending entry on vlan " << vlanID << " with ip "
          << ip.str();

  auto ndpTable = vlan->getNdpTable();
  auto entry = ndpTable->getNodeIf(ip);
  if (entry) {
    // Don't overwrite any entry with a pending entry
    return;
  }

  auto updateFn = [=](const shared_ptr<SwitchState>& state)
    -> shared_ptr<SwitchState> {
    auto* vlan = state->getVlans()->getVlanIf(vlanID).get();
    if (!vlan) {
      // This VLAN no longer exists.  Just ignore the ARP entry update.
      VLOG(4) << "VLAN " << vlanID <<
        " deleted before pending ndp entry could be added";
      return nullptr;
    }

    auto intfs = state->getInterfaces();
    auto intf = intfs->getInterfaceIf(intfID);
    if (!intf) {
      VLOG(4) << "Interface " << intfID <<
        " deleted before pending ndp entry could be added";
      return nullptr;
    }
    if (!intf->canReachAddress(ip)) {
      VLOG(4) << ip << " deleted from interface " << intfID <<
        " before pending ndp entry could be added";
      return nullptr;
    }

    shared_ptr<SwitchState> newState{state};
    auto* ndpTable = vlan->getNdpTable().get();
    auto entry = ndpTable->getNodeIf(ip);
    if (!entry) {
      // only set a pending entry when we have no entry
      ndpTable = ndpTable->modify(&vlan, &newState);
      ndpTable->addPendingEntry(ip, intfID);
    }

    VLOG(4) << "Adding pending ndp entry for " << ip.str();
    return newState;
  };

  sw_->updateState("add pending ndp entry", updateFn);
}

void IPv6Handler::updateNeighborEntry(const RxPacket* pkt,
                                      folly::IPAddressV6 ip,
                                      folly::MacAddress mac,
                                      uint32_t flags) {
  PortID port = pkt->getSrcPort();
  VlanID vlanID = pkt->getSrcVlan();

  // Look up the Vlan in the state
  auto state = sw_->getState();
  auto vlan = state->getVlans()->getVlanIf(vlanID);
  if (!vlan) {
    // Hmm, we don't actually have this VLAN configured.
    // Perhaps the state has changed since we received the packet.
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  // FIXME: Look up the correct interface based on the subnet for this IP.
  // For now we use the first interface for this VLAN.
  // We will probably just change the code to only allow a single interface
  // per VLAN.
  InterfaceID intfID(0);
  const auto& intfs = state->getInterfaces();
  const Interface* intf = nullptr;
  for (const auto& curIntf : *intfs) {
    if (curIntf->getVlanID() == vlanID) {
      intfID = curIntf->getID();
      intf = curIntf.get();
      break;
    }
  }
  if (!intf) {
    // The interface no longer exists. Just ignore the entry update.
    VLOG(1) << "Interface for vlan " << vlanID << " deleted before NDP entry "
      << ip << " --> " << mac << " could be updated";
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  // Verify the address is reachable through this interface
  if (intf->getAddressToReach(folly::IPAddress(ip))
      == intf->getAddresses().end()) {
    LOG(WARNING) << "Skip updating un-reachable neighbor entry " << ip
     << " --> " << mac << " on interface " << intfID;
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  // Check for an existing entry for this IP.
  //
  // TODO: We aren't really following the procedure recommended by RFC 4861
  // here.  We should eventually track REACHABLE/STALE/INCOMPLETE state for
  // each entry, and do the processing it describes.
  auto ndpTable = vlan->getNdpTable();
  auto entry = ndpTable->getNodeIf(ip);
  if (entry &&
      entry->getMac() == mac &&
      entry->getPort() == port &&
      entry->getIntfID() == intfID &&
      !entry->isPending()) {
    // The entry is up-to-date, so we have nothing to do.
    return;
  }

  // We do have to update the entry now.
  auto updateFn = [vlanID, port, intfID, ip, mac]
                  (const shared_ptr<SwitchState>& state) {
    // The state has changed, we need to
    // Re-validate vlan and entry, in case the state has changed
    auto* vlan = state->getVlans()->getVlanIf(vlanID).get();
    if (!vlan) {
      // This VLAN no longer exists.  Just ignore the entry update.
      VLOG(3) << "VLAN " << vlanID << " deleted before NDP entry " <<
        ip << " --> " << mac << " could be updated";
      return shared_ptr<SwitchState>();
    }

    // In case the interface subnets have changed, make sure the IP address
    // is still on a locally attached subnet
    if (!Interface::isIpAttached(ip, intfID, state)) {
      VLOG(3) << "interface subnets changed before NDP entry " <<
        ip << " --> " << mac << " could be updated";
      return shared_ptr<SwitchState>();
    }

    shared_ptr<SwitchState> newState{state};
    auto* ndpTable = vlan->getNdpTable().get();
    auto entry = ndpTable->getNodeIf(ip);

    if (!entry) {
      ndpTable = ndpTable->modify(&vlan, &newState);
      ndpTable->addEntry(ip, mac, port, intfID);
    } else {
      if (entry->getMac() == mac &&
          entry->getPort() == port &&
          entry->getIntfID() == intfID &&
          !entry->isPending()) {
        // This entry was already updated while we were waiting on the lock.
        return shared_ptr<SwitchState>();
      }
      ndpTable = ndpTable->modify(&vlan, &newState);
      ndpTable->updateEntry(ip, mac, port, intfID);
    }
    VLOG(3) << "Adding NDP entry for " << ip.str() << " --> " << mac;
    return newState;
  };

  sw_->updateState("add IPv6 neighbor", updateFn);
}

}} // facebook::fboss
