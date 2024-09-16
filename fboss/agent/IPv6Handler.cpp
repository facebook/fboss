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

#include <folly/Format.h>
#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/DHCPv6Handler.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/PacketLogger.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/NDP.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/Route.h"

#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

DECLARE_bool(intf_nbr_tables);

DEFINE_bool(
    disable_icmp_error_response,
    false,
    "Disable sending icmp error response pkts in agent");

DEFINE_bool(
    disable_neighbor_solicitation,
    false,
    "Disable sending neighbor solicitation pkts in agent");

using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::shared_ptr;
using std::unique_ptr;

namespace facebook::fboss {

IPv6Handler::IPv6Handler(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "IPv6Handler");
}
IPv6Handler::~IPv6Handler() {
  sw_->unregisterStateObserver(this);
}

template <typename BodyFn>
std::unique_ptr<TxPacket> createICMPv6Pkt(
    SwSwitch* sw,
    folly::MacAddress dstMac,
    folly::MacAddress srcMac,
    std::optional<VlanID> vlan,
    const folly::IPAddressV6& dstIP,
    const folly::IPAddressV6& srcIP,
    ICMPv6Type icmp6Type,
    ICMPv6Code icmp6Code,
    uint32_t bodyLength,
    BodyFn serializeBody) {
  IPv6Hdr ipv6(srcIP, dstIP);
  ipv6.trafficClass = kGetNetworkControlTrafficClass();
  ipv6.payloadLength = ICMPHdr::SIZE + bodyLength;
  ipv6.nextHeader = static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP);
  ipv6.hopLimit = 255;

  ICMPHdr icmp6(
      static_cast<uint8_t>(icmp6Type), static_cast<uint8_t>(icmp6Code), 0);

  uint32_t pktLen = icmp6.computeTotalLengthV6(bodyLength, vlan.has_value());
  auto pkt = sw->allocatePacket(pktLen);
  RWPrivateCursor cursor(pkt->buf());
  icmp6.serializeFullPacket(
      &cursor, dstMac, srcMac, vlan, ipv6, bodyLength, serializeBody);
  return pkt;
}

struct IPv6Handler::ICMPHeaders {
  folly::MacAddress dst;
  folly::MacAddress src;
  const IPv6Hdr* ipv6;
  const ICMPHdr* icmp6;
};

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
  return intf->routerAdvertisementSeconds() > 0;
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

template <typename VlanOrIntfT>
void IPv6Handler::handlePacket(
    unique_ptr<RxPacket> pkt,
    MacAddress dst,
    MacAddress src,
    Cursor cursor,
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf) {
  auto vlanID = getVlanIDFromVlanOrIntf(vlanOrIntf);
  auto vlanIDStr = vlanID.has_value()
      ? folly::to<std::string>(static_cast<int>(vlanID.value()))
      : "None";
  const uint32_t l3Len = pkt->getLength() - (cursor - Cursor(pkt->buf()));
  IPv6Hdr ipv6(cursor); // note: advances our cursor object
  XLOG(DBG4) << "IPv6 (" << l3Len
             << " bytes)"
                " port: "
             << pkt->getSrcPort() << " vlan: " << vlanIDStr
             << " src: " << ipv6.srcAddr.str() << " (" << src << ")"
             << " dst: " << ipv6.dstAddr.str() << " (" << dst << ")"
             << " nextHeader: " << static_cast<int>(ipv6.nextHeader);

  auto payloadLength = pkt->getLength() - (cursor - Cursor(pkt->buf()));
  if (ipv6.payloadLength > payloadLength) {
    sw_->portStats(pkt->getSrcPort())->pktBogus();
    XLOG(DBG3) << "Discarding pkt with invalid length field. length: " << " ("
               << ipv6.payloadLength << ")" << "payload length: " << " ("
               << payloadLength << ")" << " src: " << ipv6.srcAddr.str() << " ("
               << src << ")" << " dst: " << ipv6.dstAddr.str() << " (" << dst
               << ")";
    return;
  }

  // Additional data (such as FCS) may be appended after the IP payload, trim to
  // expected size of ipv6 payload
  cursor = Cursor(cursor, ipv6.payloadLength);

  // retrieve the current switch state
  auto state = sw_->getState();
  PortID port = pkt->getSrcPort();

  // NOTE: DHCPv6 solicit packet from client has hoplimit set to 1,
  // we need to handle it before send the ICMPv6 TTL exceeded
  if (ipv6.nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
    Cursor udpCursor(cursor);
    UDPHeader udpHdr;
    udpHdr.parse(&udpCursor, sw_->portStats(port));
    XLOG(DBG4) << "DHCP UDP packet, source port :" << udpHdr.srcPort
               << " destination port: " << udpHdr.dstPort;
    if (DHCPv6Handler::isForDHCPv6RelayOrServer(udpHdr)) {
      auto switchType = sw_->getSwitchInfoTable().l3SwitchType();
      if (switchType == cfg::SwitchType::VOQ) {
        // TODO(skhare)
        // Support DHCP packets for VOQ switches
        XLOG_EVERY_MS(DBG2, 5000)
            << "Dropping DHCPv6 pkt for VOQ switches, Not Supported yet";
        return;
      } else {
        DHCPv6Handler::handlePacket(
            sw_, std::move(pkt), src, dst, ipv6, udpHdr, udpCursor, vlanOrIntf);
        return;
      }
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
    // If packet is received on a lag member port, ensure that
    // LAG is in forwarding state
    if (!AggregatePort::isIngressValid(state, pkt)) {
      XLOG_EVERY_MS(DBG2, 5000)
          << "Dropping multicast ipv6 pkt ingressing on disabled agg member port "
          << pkt->getSrcPort();
      return;
    }

    // Forward multicast packet directly to corresponding host interface
    // and let Linux handle it. In software we consume ICMPv6 Multicast
    // packets for function of NDP protocol, rest all are forwarded to host.
    auto intfID = sw_->getState()->getInterfaceIDForPort(PortDescriptor(port));
    intf = state->getInterfaces()->getNodeIf(intfID);
  } else if (ipv6.dstAddr.isLinkLocal()) {
    // If srcPort == CPU port, this packet was injected by self, and then
    // trapped back via RX callback. We don't need to handle self injected
    // packets, so sete intf = nullptr;
    // However, these might be packts
    // However, we would still trigger neighbor solicitation
    // (resolveDestAndHandlePacket) if the destination IP is not already
    // resolved.
    if (port == PortID(0)) {
      intf = nullptr;
    } else {
      // Forward link-local packet directly to corresponding host interface
      // provided desAddr is assigned to that interface.
      auto intfID =
          sw_->getState()->getInterfaceIDForPort(PortDescriptor(port));
      intf = state->getInterfaces()->getNodeIf(intfID);
      if (intf && !(intf->hasAddress(ipv6.dstAddr))) {
        intf = nullptr;
      }
    }
  } else {
    // Else loopup host interface based on destAddr
    const auto iter = sw_->getAddrToLocalIntfMap().find(
        std::make_pair(RouterID(0), ipv6.dstAddr));
    if (iter != sw_->getAddrToLocalIntfMap().end()) {
      intf = interfaceMap->getNodeIf(iter->second);
    }
  }

  // If the packet is destined to us, accept packets
  // with a hop limit of 1. Else we need to forward
  // this packet so the hop limit should be at least 1
  auto minHopLimit = intf ? 0 : 1;
  if (FLAGS_send_icmp_time_exceeded && ipv6.hopLimit <= minHopLimit) {
    XLOG(DBG4) << "Rx IPv6 Packet with hop limit exceeded";
    sw_->portStats(port)->pktDropped();
    sw_->portStats(port)->ipv6HopExceeded();
    // Look up cpu mac from switchInfo
    auto switchId = sw_->getScopeResolver()->scope(port).switchId();
    MacAddress cpuMac =
        sw_->getHwAsicTable()->getHwAsicIf(switchId)->getAsicMac();
    sendICMPv6TimeExceeded(port, vlanID, cpuMac, cpuMac, ipv6, cursor);
    return;
  }

  if (intf) {
    // packets destined for us
    // Anything not handled by the controller, we will forward it to the host,
    // i.e. ping, ssh, bgp...
    if (ipv6.hopLimit == 1) {
      sw_->stats()->ipv6HopLimit1Mine();
    }
    if (ipv6.payloadLength > intf->getMtu()) {
      // Generate PTB as interface to dst intf has MTU smaller than payload
      sendICMPv6PacketTooBig(
          port, vlanID, src, dst, ipv6, intf->getMtu(), cursor);
      sw_->portStats(port)->pktDropped();
      return;
    }
    if (ipv6.nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ICMP)) {
      pkt = handleICMPv6Packet(
          std::move(pkt), dst, src, ipv6, cursor, vlanOrIntf);
      if (pkt == nullptr) {
        // packet has been handled
        return;
      }
    }

    if (sw_->sendPacketToHost(intf->getID(), std::move(pkt))) {
      sw_->portStats(port)->pktToHost(l3Len);
    } else {
      sw_->portStats(port)->pktDropped();
    }
    return;
  }

  // Don't send solicitations for multicast or broadcast addresses.
  if (!ipv6.dstAddr.isMulticast() && !ipv6.dstAddr.isLinkLocalBroadcast()) {
    // If IP is not multicast or linklocal broadcast, we need to resolve the IP
    // for this packet.
    // TODO: Add rate limiting so we don't generate too many requests for the
    // same IP.  Following the rules in RFC 4861 should be sufficient.
    resolveDestAndHandlePacket(ipv6, std::move(pkt), dst, src, cursor);
  }
}

// Explicit instantiation to avoid linker errors
// https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template void IPv6Handler::handlePacket<Vlan>(
    unique_ptr<RxPacket> pkt,
    MacAddress dst,
    MacAddress src,
    Cursor cursor,
    const std::shared_ptr<Vlan>& vlanOrIntf);

template void IPv6Handler::handlePacket<Interface>(
    unique_ptr<RxPacket> pkt,
    MacAddress dst,
    MacAddress src,
    Cursor cursor,
    const std::shared_ptr<Interface>& vlanOrIntf);

template <typename VlanOrIntfT>
unique_ptr<RxPacket> IPv6Handler::handleICMPv6Packet(
    unique_ptr<RxPacket> pkt,
    MacAddress dst,
    MacAddress src,
    const IPv6Hdr& ipv6,
    Cursor cursor,
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf) {
  ICMPHdr icmp6(cursor); // note: advances our cursor object

  // Validate the checksum, and drop the packet if it is not valid
  if (!icmp6.validateChecksum(ipv6, cursor)) {
    XLOG(DBG3) << "bad ICMPv6 checksum";
    sw_->portStats(pkt)->pktDropped();
    return nullptr;
  }

  ICMPHeaders hdr{dst, src, &ipv6, &icmp6};
  ICMPv6Type type = static_cast<ICMPv6Type>(icmp6.type);
  switch (type) {
    case ICMPv6Type::ICMPV6_TYPE_NDP_ROUTER_SOLICITATION:
      handleRouterSolicitation(std::move(pkt), hdr, cursor);
      return nullptr;
    case ICMPv6Type::ICMPV6_TYPE_NDP_ROUTER_ADVERTISEMENT:
      handleRouterAdvertisement(std::move(pkt), hdr, cursor);
      return nullptr;
    case ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION:
      handleNeighborSolicitation(std::move(pkt), hdr, cursor, vlanOrIntf);
      return nullptr;
    case ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT:
      handleNeighborAdvertisement(std::move(pkt), hdr, cursor, vlanOrIntf);
      return nullptr;
    case ICMPv6Type::ICMPV6_TYPE_NDP_REDIRECT_MESSAGE:
      sw_->portStats(pkt)->ipv6NdpPkt();
      // TODO: Do we need to bother handling this yet?
      sw_->portStats(pkt)->pktDropped();
      return nullptr;
    default:
      break;
  }

  return pkt;
}

void IPv6Handler::handleRouterSolicitation(
    unique_ptr<RxPacket> pkt,
    const ICMPHeaders& hdr,
    Cursor cursor) {
  sw_->portStats(pkt)->ipv6NdpPkt();
  if (!checkNdpPacket(hdr, pkt.get())) {
    return;
  }

  cursor.skip(4); // 4 reserved bytes

  auto intfID =
      sw_->getState()->getInterfaceIDForPort(PortDescriptor(pkt->getSrcPort()));
  auto intf = sw_->getState()->getInterfaces()->getNodeIf(intfID);
  if (!intf) {
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  MacAddress dstMac = hdr.src;
  try {
    auto ndpOptions = NDPOptions(cursor);
    if (ndpOptions.sourceLinkLayerAddress) {
      dstMac = *ndpOptions.sourceLinkLayerAddress;
    }
  } catch (const HdrParseError& e) {
    XLOG(WARNING) << e.what();
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  // Send the response
  IPAddressV6 dstIP = hdr.ipv6->srcAddr;
  if (dstIP.isZero()) {
    dstIP = IPAddressV6("ff01::1");
  }

  XLOG(DBG4) << "sending router advertisement in response to solicitation from "
             << dstIP.str() << " (" << dstMac << ")";

  uint32_t pktLen = IPv6RouteAdvertiser::getPacketSize(intf.get());
  auto resp = sw_->allocatePacket(pktLen);
  RWPrivateCursor respCursor(resp->buf());
  IPv6RouteAdvertiser::createAdvertisementPacket(
      intf.get(), &respCursor, dstMac, dstIP);
  // Based on the router solicidtation and advertisement mechanism, the
  // advertisement should send back to who request such solicidation. Besides,
  // right now, only servers send RSW router solicidation. It's kinda safe to
  // send router advertisement back to the src port.
  sw_->sendNetworkControlPacketAsync(
      std::move(resp), PortDescriptor::fromRxPacket(*pkt.get()));
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
    XLOG(DBG6) << "bad IPv6 router advertisement: source address must be "
                  "link-local: "
               << hdr.ipv6->srcAddr;
    sw_->portStats(pkt)->ipv6NdpBad();
    return;
  }

  XLOG(DBG3) << "dropping IPv6 router advertisement from " << hdr.ipv6->srcAddr;
  sw_->portStats(pkt)->pktDropped();
}

template <typename VlanOrIntfT>
void IPv6Handler::handleNeighborSolicitation(
    unique_ptr<RxPacket> pkt,
    const ICMPHeaders& hdr,
    Cursor cursor,
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf) {
  auto vlanID = getVlanIDFromVlanOrIntf(vlanOrIntf);
  auto vlanIDStr = vlanID.has_value()
      ? folly::to<std::string>(static_cast<int>(vlanID.value()))
      : "None";

  sw_->portStats(pkt)->ipv6NdpPkt();
  if (!checkNdpPacket(hdr, pkt.get())) {
    return;
  }

  cursor.skip(4); // 4 reserved bytes
  IPAddressV6 targetIP = PktUtil::readIPv6(&cursor);
  if (targetIP.isMulticast()) {
    XLOG(DBG4) << "bad IPv6 neighbor solicitation request: target is "
                  "multicast: "
               << targetIP;
    sw_->portStats(pkt)->ipv6NdpBad();
    return;
  }
  XLOG(DBG4) << "got neighbor solicitation for " << targetIP.str();

  auto state = sw_->getState();

  if (!vlanOrIntf) {
    XLOG(DBG5) << "invalid vlan or interface " << vlanIDStr
               << ", drop the packet on port: " << pkt->getSrcPort();
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  // exctract NDP options to update cache only  with value of
  // Source LinkLayer Address option, if present
  NDPOptions ndpOptions;
  try {
    ndpOptions.tryParse(cursor);
  } catch (const HdrParseError& e) {
    XLOG(DBG6) << e.what();
    sw_->portStats(pkt)->ipv6NdpBad();
    return;
  }

  auto type = ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION;

  if ((!ndpOptions.sourceLinkLayerAddress.has_value() &&
       hdr.ipv6->dstAddr.isMulticast()) ||
      (ndpOptions.sourceLinkLayerAddress.has_value() &&
       hdr.ipv6->srcAddr.isZero())) {
    /* rfc 4861 -  must not be included when the source IP address is the
      unspecified address.  must be included in multicast solicitations a
    */
    XLOG(DBG6) << "bad IPv6 neighbor solicitation request:"
               << " either multicast solicitation is missing source link layer"
               << " option or notification has source link layer address but"
               << " source is unspecified";
    sw_->portStats(pkt)->ipv6NdpBad();
    return;
  }

  if (!AggregatePort::isIngressValid(state, pkt)) {
    XLOG(DBG2) << "Dropping invalid NS ingressing on port " << pkt->getSrcPort()
               << " on vlan " << vlanIDStr << " for " << targetIP;

    return;
  }

  auto entry = vlanOrIntf->getNdpResponseTable()->getEntry(targetIP);
  auto srcPortDescriptor = PortDescriptor::fromRxPacket(*pkt.get());
  if (ndpOptions.sourceLinkLayerAddress.has_value()) {
    /* rfc 4861 - if the source address is not the unspecified address and,
    on link layers that have addresses, the solicitation includes a Source
    Link-Layer Address option, then the recipient should create or update
    the Neighbor Cache entry for the IP Source Address of the solicitation.
    */
    if (!entry) {
      // if this IP address not is in NDP response table.
      receivedNdpNotMine(
          vlanOrIntf,
          hdr.ipv6->srcAddr,
          ndpOptions.sourceLinkLayerAddress.value(),
          srcPortDescriptor,
          type,
          0);
      return;
    }

    receivedNdpMine(
        vlanOrIntf,
        hdr.ipv6->srcAddr,
        ndpOptions.sourceLinkLayerAddress.value(),
        srcPortDescriptor,
        type,
        0);
  }
  // TODO: It might be nice to support duplicate address detection, and track
  // whether our IP is tentative or not.

  // Send the response. To reply the neighbor solicitation, we can use the
  // src port of such packet to send back the neighbor advertisement.
  if (entry) {
    sendNeighborAdvertisement(
        vlanID,
        entry->getMac(),
        targetIP,
        hdr.src,
        hdr.ipv6->srcAddr,
        srcPortDescriptor);
  } else {
    XLOG(DBG2) << "Dropping NS ingressing on port " << pkt->getSrcPort()
               << " on vlan " << vlanIDStr << " for " << targetIP
               << " due to missing mac ";
  }
}

template <typename VlanOrIntfT>
void IPv6Handler::handleNeighborAdvertisement(
    unique_ptr<RxPacket> pkt,
    const ICMPHeaders& hdr,
    Cursor cursor,
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf) {
  sw_->portStats(pkt)->ipv6NdpPkt();
  if (!checkNdpPacket(hdr, pkt.get())) {
    return;
  }

  auto flags = cursor.read<uint32_t>();
  IPAddressV6 targetIP = PktUtil::readIPv6(&cursor);

  MacAddress targetMac = hdr.src;
  try {
    auto ndpOptions = NDPOptions(cursor);
    if (ndpOptions.targetLinkLayerAddress) {
      targetMac = *ndpOptions.targetLinkLayerAddress;
    }
  } catch (const HdrParseError& e) {
    XLOG(DBG3) << e.what();
    sw_->portStats(pkt)->ipv6NdpBad();
    return;
  }

  if (targetMac.isMulticast() || targetMac.isBroadcast()) {
    XLOG(DBG3) << "ignoring IPv6 neighbor advertisement for " << targetIP
               << "with multicast MAC " << targetMac;
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  if (!vlanOrIntf) {
    // Hmm, we don't actually have this VLAN configured.
    // Perhaps the state has changed since we received the packet.
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  if (!AggregatePort::isIngressValid(sw_->getState(), pkt, true)) {
    // drop NDP advertisement packets when LAG port is not up,
    // otherwise, NDP entry would be created for this down port,
    // and confuse later neighbor/next hop resolution logics
    auto vlanID = getVlanIDFromVlanOrIntf(vlanOrIntf);
    auto vlanIDStr = vlanID.has_value()
        ? folly::to<std::string>(static_cast<int>(vlanID.value()))
        : "None";
    XLOG(DBG2) << "Dropping invalid neighbor advertisement ingressing on port "
               << pkt->getSrcPort() << " on vlan " << vlanIDStr << " for "
               << targetIP;
    sw_->portStats(pkt)->pktDropped();
    return;
  }

  XLOG(DBG4) << "got neighbor advertisement for " << targetIP << " ("
             << targetMac << ")";

  auto type = ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT;

  const auto& dstIP = hdr.ipv6->dstAddr;
  /*
   * Unsolicited nbr adv are sent to mcast (typically all nodes mcast) address.
   * RFC (rfc2461)  explicitly allows for unsolicited ndp advertisments
   */
  if (!dstIP.isMulticast()) {
    // Check to see if this IP address is in our NDP response table.
    auto entry = vlanOrIntf->getNdpResponseTable()->getEntry(hdr.ipv6->dstAddr);
    if (!entry) {
      receivedNdpNotMine(
          vlanOrIntf,
          targetIP,
          hdr.src,
          PortDescriptor::fromRxPacket(*pkt.get()),
          type,
          flags);
      return;
    }
  }

  receivedNdpMine(
      vlanOrIntf,
      targetIP,
      hdr.src,
      PortDescriptor::fromRxPacket(*pkt.get()),
      type,
      flags);
}

void IPv6Handler::sendICMPv6TimeExceeded(
    PortID srcPort,
    std::optional<VlanID> srcVlan,
    MacAddress dst,
    MacAddress src,
    IPv6Hdr& v6Hdr,
    folly::io::Cursor cursor) {
  if (FLAGS_disable_icmp_error_response) {
    XLOG(DBG4)
        << "skipping sending icmpv6 time exceeded since icmp error response generation is disabled";
    return;
  }
  auto state = sw_->getState();

  /*
   * The payload of ICMPv6TimeExceeded consists of:
   *  - The unused field of the ICMP header;
   *  - The original IPv6 header and its payload.
   */
  uint32_t icmpPayloadLength = (uint32_t)ICMPHdr::ICMPV6_UNUSED_LEN +
      (uint32_t)IPv6Hdr::SIZE + cursor.totalLength();
  // This payload and the IPv6/ICMPv6 headers must fit the IPv6 MTU
  icmpPayloadLength = std::min(
      icmpPayloadLength,
      (uint32_t)IPV6_MIN_MTU - (uint32_t)IPv6Hdr::SIZE -
          (uint32_t)ICMPHdr::SIZE);

  auto serializeBody = [&](RWPrivateCursor* sendCursor) {
    // ICMPv6 unused field
    sendCursor->writeBE<uint32_t>(0);
    v6Hdr.serialize(sendCursor);
    auto remainingLength =
        icmpPayloadLength - ICMPHdr::ICMPV6_UNUSED_LEN - IPv6Hdr::SIZE;
    sendCursor->push(cursor, remainingLength);
  };

  IPAddressV6 srcIp;
  try {
    srcIp = getSwitchIntfIPv6(
        state, sw_->getState()->getInterfaceIDForPort(PortDescriptor(srcPort)));
  } catch (const std::exception&) {
    srcIp = getAnyIntfIPv6(state);
  }

  auto icmpPkt = createICMPv6Pkt(
      sw_,
      dst,
      src,
      srcVlan,
      v6Hdr.srcAddr,
      srcIp,
      ICMPv6Type::ICMPV6_TYPE_TIME_EXCEEDED,
      ICMPv6Code::ICMPV6_CODE_TIME_EXCEEDED_HOPLIMIT_EXCEEDED,
      icmpPayloadLength,
      serializeBody);

  auto srcVlanStr = srcVlan.has_value()
      ? folly::to<std::string>(static_cast<int>(srcVlan.value()))
      : "None";
  XLOG(DBG4) << "sending ICMPv6 Time Exceeded with srcMac  " << src
             << " dstMac: " << dst << " vlan: " << srcVlanStr
             << " dstIp: " << v6Hdr.srcAddr.str() << " srcIP: " << srcIp.str()
             << " bodyLength: " << icmpPayloadLength;
  sw_->sendPacketSwitchedAsync(std::move(icmpPkt));
}

void IPv6Handler::sendICMPv6PacketTooBig(
    PortID srcPort,
    std::optional<VlanID> srcVlan,
    folly::MacAddress dst,
    folly::MacAddress src,
    IPv6Hdr& v6Hdr,
    int expectedMtu,
    folly::io::Cursor cursor) {
  if (FLAGS_disable_icmp_error_response) {
    XLOG(DBG4)
        << "skipping sending icmpv6 time exceeded since icmp error response generation is disabled";
    return;
  }
  auto state = sw_->getState();

  // payload serialization function
  // 4 bytes expected MTU + ipv6 header + as much payload as possible to fit MTU
  // this is upper limit of bodyLength
  uint32_t bodyLengthLimit = IPV6_MIN_MTU - ICMPHdr::computeTotalLengthV6(0);
  // this is when we add the whole input L3 packet
  uint32_t fullPacketLength = (uint32_t)ICMPHdr::ICMPV6_MTU_LEN +
      (uint32_t)IPv6Hdr::SIZE + cursor.totalLength();
  auto bodyLength = std::min(bodyLengthLimit, fullPacketLength);

  auto serializeBody = [&](RWPrivateCursor* sendCursor) {
    sendCursor->writeBE<uint32_t>(expectedMtu);
    v6Hdr.serialize(sendCursor);
    auto remainingLength =
        bodyLength - IPv6Hdr::SIZE - ICMPHdr::ICMPV6_UNUSED_LEN;
    sendCursor->push(cursor, remainingLength);
  };

  IPAddressV6 srcIp = getSwitchIntfIPv6(
      state, sw_->getState()->getInterfaceIDForPort(PortDescriptor(srcPort)));
  auto icmpPkt = createICMPv6Pkt(
      sw_,
      dst,
      src,
      srcVlan,
      v6Hdr.srcAddr,
      srcIp,
      ICMPv6Type::ICMPV6_TYPE_PACKET_TOO_BIG,
      ICMPv6Code::ICMPV6_CODE_PACKET_TOO_BIG,
      bodyLength,
      serializeBody);

  auto srcVlanStr = srcVlan.has_value()
      ? folly::to<std::string>(static_cast<int>(srcVlan.value()))
      : "None";
  XLOG(DBG4) << "sending ICMPv6 Packet Too Big with srcMac  " << src
             << " dstMac: " << dst << " vlan: " << srcVlanStr
             << " dstIp: " << v6Hdr.srcAddr.str() << " srcIP: " << srcIp.str()
             << " bodyLength: " << bodyLength;
  sw_->sendPacketSwitchedAsync(std::move(icmpPkt));
  sw_->portStats(srcPort)->pktTooBig();
}

bool IPv6Handler::checkNdpPacket(const ICMPHeaders& hdr, const RxPacket* pkt)
    const {
  // Validation common for all NDP packets
  if (hdr.ipv6->hopLimit != 255) {
    XLOG(DBG3) << "bad IPv6 NDP request (" << hdr.icmp6->type
               << "): hop limit should be 255, received value is "
               << static_cast<int>(hdr.ipv6->hopLimit);
    sw_->portStats(pkt)->ipv6NdpBad();
    return false;
  }
  if (hdr.icmp6->code != 0) {
    XLOG(DBG3) << "bad IPv6 NDP request (" << hdr.icmp6->type
               << "): code should be 0, received value is " << hdr.icmp6->code;
    sw_->portStats(pkt)->ipv6NdpBad();
    return false;
  }

  return true;
}

void IPv6Handler::sendMulticastNeighborSolicitation(
    SwSwitch* sw,
    const IPAddressV6& targetIP,
    const MacAddress& srcMac,
    const std::optional<VlanID>& vlanID) {
  if (FLAGS_disable_neighbor_updates) {
    XLOG(DBG4)
        << "skipping sending neighbor solicitation since neighbor updates are disabled";
    return;
  }

  IPAddressV6 solicitedNodeAddr = targetIP.getSolicitedNodeAddress();
  MacAddress dstMac = MacAddress::createMulticast(solicitedNodeAddr);
  // For now, we always use our link local IP as the source.
  IPAddressV6 srcIP(IPAddressV6::LINK_LOCAL, srcMac);

  NDPOptions ndpOptions;
  ndpOptions.sourceLinkLayerAddress.emplace(srcMac);

  auto portDescriptor = getInterfacePortDescriptorToReach(sw, targetIP);

  auto portDescriptorStr =
      portDescriptor.has_value() ? portDescriptor.value().str() : "None";
  auto vlanIDStr = vlanID.has_value()
      ? folly::to<std::string>(static_cast<int>(vlanID.value()))
      : "None";
  XLOG(DBG4) << "sending neighbor solicitation for " << targetIP << " on vlan "
             << vlanIDStr << " solicitedNodeAddr: " << solicitedNodeAddr.str()
             << " using port: " << portDescriptorStr;

  sendNeighborSolicitation(
      sw,
      solicitedNodeAddr,
      dstMac,
      srcIP,
      srcMac,
      targetIP,
      vlanID,
      portDescriptor,
      ndpOptions);
}

/* unicast neighbor solicitation */
void IPv6Handler::sendUnicastNeighborSolicitation(
    SwSwitch* sw,
    const folly::IPAddressV6& targetIP,
    const folly::MacAddress& targetMac,
    const folly::IPAddressV6& srcIP,
    const folly::MacAddress& srcMac,
    const std::optional<VlanID>& vlanID,
    const PortDescriptor& portDescriptor) {
  auto portToSend{portDescriptor};

  if (FLAGS_disable_neighbor_updates) {
    XLOG(DBG4)
        << "skipping sending neighbor solicitation since neighbor updates are disabled";
    return;
  }
  if (FLAGS_disable_neighbor_solicitation) {
    XLOG(DBG4)
        << "skipping sending neighbor solicitation since flag disable_neighbor_solicitation set to true";
    return;
  }

  auto state = sw->getState();

  InterfaceID intfID;
  switch (portDescriptor.type()) {
    case PortDescriptor::PortType::PHYSICAL:
      intfID = sw->getState()->getInterfaceIDForPort(portDescriptor);
      break;
    case PortDescriptor::PortType::AGGREGATE:
      intfID = sw->getState()->getInterfaceIDForPort(portDescriptor);
      break;
    case PortDescriptor::PortType::SYSTEM_PORT:
      auto physPortID = getPortID(portDescriptor.sysPortID(), sw->getState());
      portToSend = PortDescriptor(physPortID);
      intfID = sw->getState()->getInterfaceIDForPort(portToSend);
  }

  if (!Interface::isIpAttached(targetIP, intfID, state)) {
    XLOG(DBG2) << "unicast neighbor solicitation not sent, neighbor address: "
               << targetIP << ", is not in the subnets of interface: "
               << " for interface:" << intfID;
    return;
  }

  XLOG(DBG4) << "sending unicast neighbor solicitation to " << targetIP << "("
             << targetMac << ")" << " on interface " << intfID << " from "
             << srcIP << "(" << srcMac
             << ")  portDescriptor:" << portDescriptor.str()
             << " portToSend: " << portToSend.str();

  return sendNeighborSolicitation(
      sw, targetIP, targetMac, srcIP, srcMac, targetIP, vlanID, portToSend);
}

void IPv6Handler::sendMulticastNeighborSolicitation(
    SwSwitch* sw,
    const IPAddressV6& targetIP) {
  auto intf =
      sw->getState()->getInterfaces()->getIntfToReach(RouterID(0), targetIP);

  if (!intf) {
    XLOG(DBG0) << "Cannot find interface for " << targetIP;
    return;
  }

  sendMulticastNeighborSolicitation(
      sw, targetIP, intf->getMac(), intf->getVlanIDIf());
}

void IPv6Handler::resolveDestAndHandlePacket(
    IPv6Hdr hdr,
    unique_ptr<RxPacket> pkt,
    MacAddress dst,
    MacAddress src,
    Cursor cursor) {
  // Right now this either responds with PTB or generate neighbor soliciations
  auto ingressPort = pkt->getSrcPort();
  auto vlanID = pkt->getSrcVlanIf();
  auto targetIP = hdr.dstAddr;
  auto state = sw_->getState();

  if (vlanID.has_value()) {
    auto ingressInterface =
        state->getInterfaces()->getInterfaceInVlanIf(vlanID.value());
    if (!ingressInterface) {
      // Received packed on unknown VLAN
      return;
    }
  }

  auto route = sw_->longestMatch(state, targetIP, RouterID(0));
  if (!route || !route->isResolved()) {
    sw_->portStats(ingressPort)->ipv6DstLookupFailure();
    // No way to reach targetIP
    return;
  }

  auto interfaces = state->getInterfaces();
  auto nexthops = route->getForwardInfo().getNextHopSet();

  for (auto nexthop : nexthops) {
    // get interface needed to reach next hop
    auto intf = interfaces->getNodeIf(nexthop.intf());
    if (intf) {
      // what should be source & destination of packet
      auto source = intf->getAddressToReach(nexthop.addr())->first.asV6();
      auto target = route->isConnected() ? targetIP : nexthop.addr().asV6();

      if (source == target) {
        // This packet is for us.  Don't generate PTB or NDP request.
        continue;
      }

      if (hdr.payloadLength > intf->getMtu()) {
        // Generate PTB as interface to next hop has MTU smaller than payload
        sendICMPv6PacketTooBig(
            ingressPort, vlanID, src, dst, hdr, intf->getMtu(), cursor);
        sw_->portStats(ingressPort)->pktDropped();
        return;
      } else {
        // Check if destination is unknown, in which case trigger NDP
        auto entry = getNeighborEntryForIP<NdpEntry>(
            state, intf, target, FLAGS_intf_nbr_tables);

        if (nullptr == entry) {
          // No entry in NDP table, create a neighbor solicitation packet
          sendMulticastNeighborSolicitation(
              sw_, target, intf->getMac(), intf->getVlanIDIf());
          // Notify the updater that we sent a solicitation out
          sw_->sentNeighborSolicitation(intf, target);
        } else {
          XLOG(DBG5) << "not sending neighbor solicitation for " << target.str()
                     << ", " << ((entry->isPending()) ? "pending" : "")
                     << " entry already exists";
        }
      }
    }
  }
  sw_->portStats(pkt)->pktDropped();
}

void IPv6Handler::sendMulticastNeighborSolicitations(
    PortID ingressPort,
    const folly::IPAddressV6& targetIP) {
  // Don't send solicitations for multicast or broadcast addresses.
  if (targetIP.isMulticast() || targetIP.isLinkLocalBroadcast()) {
    return;
  }

  auto state = sw_->getState();

  auto route = sw_->longestMatch(state, targetIP, RouterID(0));
  if (!route || !route->isResolved()) {
    sw_->portStats(ingressPort)->ipv6DstLookupFailure();
    // No way to reach targetIP
    return;
  }

  auto intfs = state->getInterfaces();
  auto nhs = route->getForwardInfo().getNextHopSet();
  for (auto nh : nhs) {
    auto intf = intfs->getNodeIf(nh.intf());
    if (intf) {
      if (nh.addr().isV6()) {
        auto source = intf->getAddressToReach(nh.addr())->first.asV6();
        auto target = route->isConnected() ? targetIP : nh.addr().asV6();
        if (source == target) {
          // This packet is for us.  Don't send NDP requests to ourself.
          continue;
        }

        sw_->sendNdpSolicitationHelper(intf, state, target);

      } else if (nh.addr().isV4() && !route->isConnected()) {
        auto source = intf->getAddressToReach(nh.addr())->first.asV4();
        auto target = nh.addr().asV4();
        if (source == target) {
          // This packet is for us.  Don't send ARP requess for our own IP.
          continue;
        }

        sw_->sendArpRequestHelper(intf, state, source, target);
      }
    }
  }
}

void IPv6Handler::floodNeighborAdvertisements() {
  for (const auto& [_, intfMap] :
       std::as_const(*sw_->getState()->getInterfaces())) {
    for (auto iter : std::as_const(*intfMap)) {
      // This check is mostly for agent tests where we dont want to flood NDP
      // causing loop, when ports are in loopback
      const auto& intf = iter.second;
      if (isAnyInterfacePortInLoopbackMode(sw_->getState(), intf)) {
        XLOG(DBG2) << "Do not flood neighbor advertisement on interface: "
                   << intf->getName();
        continue;
      }

      // If NDP is flooded on recycle port interface, it will be resolved and
      // will get added as DYNAMIC entry, which is incorrect.
      if (isAnyInterfacePortRecyclePort(sw_->getState(), intf)) {
        XLOG(DBG2)
            << "Do not flood neighbor advertisement on recycle port interface: "
            << intf->getName();
        continue;
      }

      for (auto iter : std::as_const(*intf->getAddresses())) {
        auto addrEntry = folly::IPAddress(iter.first);
        if (!addrEntry.isV6()) {
          continue;
        }

        std::optional<PortDescriptor> portDescriptor{std::nullopt};
        auto switchType = sw_->getSwitchInfoTable().l3SwitchType();
        if (switchType == cfg::SwitchType::VOQ) {
          // VOQ switches don't use VLANs (no broadcast domain).
          // Find the port to send out the pkt with pipeline bypass on.
          CHECK(intf->getSystemPortID().has_value());
          portDescriptor = PortDescriptor(
              getPortID(*intf->getSystemPortID(), sw_->getState()));
          XLOG(DBG4) << "Sending neighbor advertisements for "
                     << addrEntry.str()
                     << " Using port: " << portDescriptor.value().str();
        }

        sendNeighborAdvertisement(
            intf->getVlanIDIf(),
            intf->getMac(),
            addrEntry.asV6(),
            MacAddress::BROADCAST,
            IPAddressV6(),
            portDescriptor);
      }
    }
  }
}

void IPv6Handler::sendNeighborAdvertisement(
    std::optional<VlanID> vlan,
    MacAddress srcMac,
    IPAddressV6 srcIP,
    MacAddress dstMac,
    IPAddressV6 dstIP,
    const std::optional<PortDescriptor>& portDescriptor) {
  if (FLAGS_disable_neighbor_updates) {
    XLOG(DBG4)
        << "skipping sending neighbor advertisement since neighbor updates are disabled";
    return;
  }
  XLOG(DBG4) << "sending neighbor advertisement to " << dstIP.str() << " ("
             << dstMac << "): for " << srcIP << " (" << srcMac << ")";

  uint32_t flags =
      NeighborAdvertisementFlags::ROUTER | NeighborAdvertisementFlags::OVERRIDE;
  if (dstIP.isZero()) {
    // TODO: add a constructor that doesn't require string processing
    dstIP = IPAddressV6("ff01::1");
  } else {
    flags |= NeighborAdvertisementFlags::SOLICITED;
  }

  NDPOptions ndpOptions;
  ndpOptions.targetLinkLayerAddress.emplace(srcMac);

  uint32_t bodyLength = ICMPHdr::ICMPV6_UNUSED_LEN + IPAddressV6::byteCount() +
      ndpOptions.computeTotalLength();

  auto serializeBody = [&](RWPrivateCursor* cursor) {
    cursor->writeBE<uint32_t>(flags);
    cursor->push(srcIP.bytes(), IPAddressV6::byteCount());
    ndpOptions.serialize(cursor);
  };

  auto pkt = createICMPv6Pkt(
      sw_,
      dstMac,
      srcMac,
      vlan,
      dstIP,
      srcIP,
      ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
      ICMPv6Code::ICMPV6_CODE_NDP_MESSAGE_CODE,
      bodyLength,
      serializeBody);
  sw_->sendNetworkControlPacketAsync(std::move(pkt), portDescriptor);
}

void IPv6Handler::sendNeighborSolicitation(
    SwSwitch* sw,
    const folly::IPAddressV6& dstIP,
    const folly::MacAddress& dstMac,
    const folly::IPAddressV6& srcIP,
    const folly::MacAddress& srcMac,
    const folly::IPAddressV6& neighborIP,
    const std::optional<VlanID>& vlanID,
    const std::optional<PortDescriptor>& portDescriptor,
    const NDPOptions& ndpOptions) {
  sw->getPacketLogger()->log(
      "NDP", "TX", vlanID, srcMac.toString(), srcIP.str(), neighborIP.str());
  auto state = sw->getState();

  uint32_t bodyLength = ICMPHdr::ICMPV6_UNUSED_LEN + IPAddressV6::byteCount() +
      ndpOptions.computeTotalLength();

  auto serializeBody = [&](RWPrivateCursor* cursor) {
    cursor->writeBE<uint32_t>(0); // reserved
    cursor->push(neighborIP.bytes(), IPAddressV6::byteCount());
    ndpOptions.serialize(cursor);
  };

  auto pkt = createICMPv6Pkt(
      sw,
      dstMac,
      srcMac,
      vlanID,
      dstIP,
      srcIP,
      ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_SOLICITATION,
      ICMPv6Code::ICMPV6_CODE_NDP_MESSAGE_CODE,
      bodyLength,
      serializeBody);
  sw->sendNetworkControlPacketAsync(std::move(pkt), portDescriptor);
}

template <typename VlanOrIntfT>
void IPv6Handler::receivedNdpNotMine(
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf,
    IPAddressV6 ip,
    MacAddress macAddr,
    PortDescriptor port,
    ICMPv6Type type,
    uint32_t flags) {
  sw_->stats()->ipv6NdpNotMine();
  auto updater = sw_->getNeighborUpdater();
  if constexpr (std::is_same_v<VlanOrIntfT, Vlan>) {
    updater->receivedNdpNotMine(
        vlanOrIntf->getID(), ip, macAddr, port, type, flags);

  } else {
    updater->receivedNdpNotMineForIntf(
        vlanOrIntf->getID(), ip, macAddr, port, type, flags);
  }
}

template <typename VlanOrIntfT>
void IPv6Handler::receivedNdpMine(
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf,
    IPAddressV6 ip,
    MacAddress macAddr,
    PortDescriptor port,
    ICMPv6Type type,
    uint32_t flags) {
  auto updater = sw_->getNeighborUpdater();
  if constexpr (std::is_same_v<VlanOrIntfT, Vlan>) {
    updater->receivedNdpMine(
        vlanOrIntf->getID(), ip, macAddr, port, type, flags);

  } else {
    updater->receivedNdpMineForIntf(
        vlanOrIntf->getID(), ip, macAddr, port, type, flags);
  }
}

std::optional<PortDescriptor> IPv6Handler::getInterfacePortDescriptorToReach(
    SwSwitch* sw,
    const folly::IPAddressV6& ipAddr) {
  std::optional<PortDescriptor> portDescriptor{std::nullopt};

  if (sw->getSwitchInfoTable().l3SwitchType() == cfg::SwitchType::VOQ) {
    // VOQ switches don't use VLANs (no broadcast domain).
    // Find the port to send out the pkt with pipeline bypass on.
    auto portID = getInterfacePortToReach(sw->getState(), ipAddr);
    if (portID.has_value()) {
      portDescriptor = PortDescriptor(portID.value());
    }
  }

  return portDescriptor;
}

} // namespace facebook::fboss
