/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ArpHandler.h"

#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/PacketLogger.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

using folly::IPAddressV4;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::shared_ptr;
using std::unique_ptr;

enum : uint16_t {
  ARP_HTYPE_ETHERNET = 1,
  ARP_PTYPE_IPV4 = 0x0800,
};

enum : uint8_t {
  ARP_HLEN_ETHERNET = 6,
  ARP_PLEN_IPV4 = 4,
};

namespace facebook::fboss {

ArpHandler::ArpHandler(SwSwitch* sw) : sw_(sw) {}

template <typename VlanOrIntfT>
void ArpHandler::handlePacket(
    unique_ptr<RxPacket> pkt,
    MacAddress /*dst*/,
    MacAddress /*src*/,
    Cursor cursor,
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf) {
  auto stats = sw_->stats();
  CHECK(stats);
  PortID port = pkt->getSrcPort();
  CHECK(stats->port(port));

  auto vlanID = getVlanIDFromVlanOrIntf(vlanOrIntf);
  auto vlanIDStr = vlanID.has_value()
      ? folly::to<std::string>(static_cast<int>(vlanID.value()))
      : "None";

  stats->port(port)->arpPkt();
  // Read htype, ptype, hlen, and plen
  auto htype = cursor.readBE<uint16_t>();
  if (htype != ARP_HTYPE_ETHERNET) {
    stats->port(port)->arpUnsupported();
    return;
  }
  auto ptype = cursor.readBE<uint16_t>();
  if (ptype != ARP_PTYPE_IPV4) {
    stats->port(port)->arpUnsupported();
    return;
  }
  auto hlen = cursor.readBE<uint8_t>();
  if (hlen != ARP_HLEN_ETHERNET) {
    stats->port(port)->arpUnsupported();
    return;
  }
  auto plen = cursor.readBE<uint8_t>();
  if (plen != ARP_PLEN_IPV4) {
    stats->port(port)->arpUnsupported();
    return;
  }

  if (!vlanOrIntf) {
    // Hmm, we don't actually have this VLAN configured.
    // Perhaps the state has changed since we received the packet.
    stats->port(port)->pktDropped();
    return;
  }

  // Parse the remaining data in the packet
  auto readOp = cursor.readBE<uint16_t>();
  auto senderMac = PktUtil::readMac(&cursor);
  auto senderIP = PktUtil::readIPv4(&cursor);
  auto targetMac = PktUtil::readMac(&cursor);
  auto targetIP = PktUtil::readIPv4(&cursor);

  if (readOp != ARP_OP_REQUEST && readOp != ARP_OP_REPLY) {
    stats->port(port)->arpBadOp();
    return;
  }

  auto op = ArpOpCode(readOp);

  // Check to see if this IP address is in our ARP response table.
  auto entry = vlanOrIntf->getArpResponseTable()->getEntry(targetIP);
  if (!entry) {
    // The target IP does not refer to us.
    XLOG(DBG5) << "ignoring ARP message for " << targetIP.str() << " on vlan "
               << vlanIDStr;
    stats->port(port)->arpNotMine();

    receivedArpNotMine(
        vlanOrIntf,
        senderIP,
        senderMac,
        PortDescriptor::fromRxPacket(*pkt.get()),
        op);
    return;
  }

  if (op == ARP_OP_REQUEST) {
    stats->port(port)->arpRequestRx();
  } else {
    CHECK(op == ARP_OP_REPLY);
    stats->port(port)->arpReplyRx();
  }

  if (op == ARP_OP_REQUEST &&
      !AggregatePort::isIngressValid(sw_->getState(), pkt)) {
    XLOG(DBG2) << "Dropping invalid ARP request ingressing on port "
               << pkt->getSrcPort() << " on vlan " << vlanIDStr << " for "
               << targetIP;
    return;
  }

  // This ARP packet is destined to us.
  // Update the sender IP --> sender MAC entry in the ARP table.
  receivedArpMine(
      vlanOrIntf,
      senderIP,
      senderMac,
      PortDescriptor::fromRxPacket(*pkt.get()),
      op);

  // Send a reply if this is an ARP request.
  if (op == ARP_OP_REQUEST) {
    sendArpReply(
        vlanID,
        pkt->getSrcPort(),
        entry->getMac(),
        targetIP,
        senderMac,
        senderIP);
  }

  (void)targetMac; // unused
}

// Explicit instantiation to avoid linker errors
// https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template void ArpHandler::handlePacket<Vlan>(
    unique_ptr<RxPacket> pkt,
    MacAddress /*dst*/,
    MacAddress /*src*/,
    Cursor cursor,
    const std::shared_ptr<Vlan>& vlanOrIntf);

template void ArpHandler::handlePacket<Interface>(
    unique_ptr<RxPacket> pkt,
    MacAddress /*dst*/,
    MacAddress /*src*/,
    Cursor cursor,
    const std::shared_ptr<Interface>& vlanOrIntf);

static void sendArp(
    SwSwitch* sw,
    std::optional<VlanID> vlan,
    ArpOpCode op,
    MacAddress senderMac,
    IPAddressV4 senderIP,
    MacAddress targetMac,
    IPAddressV4 targetIP,
    const std::optional<PortDescriptor>& portDesc = std::nullopt) {
  auto vlanStr = vlan.has_value()
      ? folly::to<std::string>(static_cast<int>(vlan.value()))
      : "None";
  XLOG(DBG4) << "sending ARP " << ((op == ARP_OP_REQUEST) ? "request" : "reply")
             << " on vlan " << vlanStr << " to " << targetIP.str() << " ("
             << targetMac << "): " << senderIP.str() << " is " << senderMac;

  // TODO: We need a more robust mechanism for setting up the ethernet
  // header in the response.  The HwSwitch should probably be responsible for
  // setting it up, and determinine whether or not a VLAN tag needs to be
  // present.
  //
  // The minimum packet length is 64.  We use 68 here on the assumption that
  // the packet will go out untagged, which will remove 4 bytes.
  uint32_t pktLen = 68;
  auto pkt = sw->allocatePacket(pktLen);
  RWPrivateCursor cursor(pkt->buf());

  pkt->writeEthHeader(
      &cursor, targetMac, senderMac, vlan, ArpHandler::ETHERTYPE_ARP);
  cursor.writeBE<uint16_t>(ARP_HTYPE_ETHERNET);
  cursor.writeBE<uint16_t>(ARP_PTYPE_IPV4);
  cursor.writeBE<uint8_t>(ARP_HLEN_ETHERNET);
  cursor.writeBE<uint8_t>(ARP_PLEN_IPV4);
  cursor.writeBE<uint16_t>(op);
  cursor.push(senderMac.bytes(), MacAddress::SIZE);
  cursor.write<uint32_t>(senderIP.toLong());
  cursor.push(
      ((op == ARP_OP_REQUEST) ? MacAddress::ZERO.bytes() : targetMac.bytes()),
      MacAddress::SIZE);
  cursor.write<uint32_t>(targetIP.toLong());
  // Fill the padding with 0s
  memset(cursor.writableData(), 0, cursor.length());

  sw->sendNetworkControlPacketAsync(std::move(pkt), portDesc);
}

void ArpHandler::floodGratuituousArp() {
  for (const auto& [_, intfMap] :
       std::as_const(*sw_->getState()->getInterfaces())) {
    for (auto iiter : std::as_const(*intfMap)) {
      const auto& intf = iiter.second;
      // mostly for agent tests where we dont want to flood arp
      // causing loop, when ports are in loopback
      if (isAnyInterfacePortInLoopbackMode(sw_->getState(), intf)) {
        XLOG(DBG2) << "Do not flood gratuituous arp on interface: "
                   << intf->getName();
        continue;
      }
      for (auto iter : std::as_const(*intf->getAddresses())) {
        auto addrEntry = folly::IPAddress(iter.first);
        if (!addrEntry.isV4()) {
          continue;
        }
        auto v4Addr = addrEntry.asV4();
        // Gratuitous arps have both source and destination IPs set to
        // originator's address
        sendArp(
            sw_,
            intf->getVlanIDIf(),
            ARP_OP_REQUEST,
            intf->getMac(),
            v4Addr,
            MacAddress::BROADCAST,
            v4Addr);
      }
    }
  }
}

void ArpHandler::sendArpReply(
    std::optional<VlanID> vlan,
    PortID port,
    MacAddress senderMac,
    IPAddressV4 senderIP,
    MacAddress targetMac,
    IPAddressV4 targetIP) {
  sw_->portStats(port)->arpReplyTx();
  // Before sending Arp reply, we've already programmed the IP to be reachable
  // over this port with the given mac by updater->receivedArpMine(). This is
  // what makes our assumption of sending ArpReply out of the same port
  // kind of safe.
  sendArp(
      sw_,
      vlan,
      ARP_OP_REPLY,
      senderMac,
      senderIP,
      targetMac,
      targetIP,
      PortDescriptor(port));
}

void ArpHandler::sendArpRequest(
    SwSwitch* sw,
    std::optional<VlanID> vlanID,
    const MacAddress& srcMac,
    const IPAddressV4& senderIP,
    const IPAddressV4& targetIP) {
  sw->getPacketLogger()->log(
      "ARP", "TX", vlanID, srcMac.toString(), senderIP.str(), targetIP.str());
  sw->stats()->arpRequestTx();
  sendArp(
      sw,
      vlanID,
      ARP_OP_REQUEST,
      srcMac,
      senderIP,
      MacAddress::BROADCAST,
      targetIP);
}

void ArpHandler::sendArpRequest(
    SwSwitch* sw,
    const folly::IPAddressV4& targetIP) {
  auto intf =
      sw->getState()->getInterfaces()->getIntfToReach(RouterID(0), targetIP);

  if (!intf) {
    XLOG(DBG0) << "Cannot find interface for " << targetIP;
    return;
  }

  auto addrToReach = intf->getAddressToReach(targetIP);

  sendArpRequest(
      sw,
      intf->getVlanIDIf(),
      intf->getMac(),
      addrToReach->first.asV4(),
      targetIP);
}

template <typename VlanOrIntfT>
void ArpHandler::receivedArpNotMine(
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf,
    IPAddressV4 ip,
    MacAddress mac,
    PortDescriptor port,
    ArpOpCode op) {
  auto updater = sw_->getNeighborUpdater();
  if constexpr (std::is_same_v<VlanOrIntfT, Vlan>) {
    updater->receivedArpNotMine(vlanOrIntf->getID(), ip, mac, port, op);
  } else {
    updater->receivedArpNotMineForIntf(vlanOrIntf->getID(), ip, mac, port, op);
  }
}

template <typename VlanOrIntfT>
void ArpHandler::receivedArpMine(
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf,
    IPAddressV4 ip,
    MacAddress mac,
    PortDescriptor port,
    ArpOpCode op) {
  auto updater = sw_->getNeighborUpdater();
  if constexpr (std::is_same_v<VlanOrIntfT, Vlan>) {
    updater->receivedArpMine(vlanOrIntf->getID(), ip, mac, port, op);
  } else {
    updater->receivedArpMineForIntf(vlanOrIntf->getID(), ip, mac, port, op);
  }
}

} // namespace facebook::fboss
