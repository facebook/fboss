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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Vlan.h"

using folly::IPAddressV4;
using folly::MacAddress;
using folly::io::Cursor;
using folly::io::RWPrivateCursor;
using std::unique_ptr;
using std::shared_ptr;

enum : uint16_t {
  ARP_HTYPE_ETHERNET = 1,
  ARP_PTYPE_IPV4 = 0x0800,
};

enum : uint8_t {
  ARP_HLEN_ETHERNET = 6,
  ARP_PLEN_IPV4 = 4,
};

namespace facebook { namespace fboss {

ArpHandler::ArpHandler(SwSwitch* sw)
  : sw_(sw) {
}

void ArpHandler::handlePacket(unique_ptr<RxPacket> pkt,
                              MacAddress dst,
                              MacAddress src,
                              Cursor cursor) {
  PortID port = pkt->getSrcPort();
  auto stats = sw_->stats();
  stats->port(port)->arpPkt();

  // Look up the Vlan state.
  auto state = sw_->getState();
  auto vlan = state->getVlans()->getVlanIf(pkt->getSrcVlan());
  if (!vlan) {
    // Hmm, we don't actually have this VLAN configured.
    // Perhaps the state has changed since we received the packet.
    stats->port(port)->pktDropped();
    return;
  }

	const uint32_t l2Len = pkt->getLength();
	/*
	 * Short circuit remaining logic if we're letting the host OS handle
	 * the packet. TODO Should probably filter by MAC address here first:
	 * -- if Interface's MAC
	 * -- if broadcast MAC
	 * -- if multicast MAC
	 */
	if (sw_->runningInNetlinkMode()) {
		if (sw_->sendPacketToHost(std::move(pkt))) {
			stats->port(port)->pktToHost(l2Len);
		} else {
			stats->port(port)->pktDropped();
		}
		VLOG(2) << "Sent ARP packet to host";
		return;
	}

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

  auto updater = sw_->getNeighborUpdater();
  // Check to see if this IP address is in our ARP response table.
  auto entry = vlan->getArpResponseTable()->getEntry(targetIP);
  if (!entry) {
    // The target IP does not refer to us.
    VLOG(5) << "ignoring ARP message for " << targetIP.str()
            << " on vlan " << pkt->getSrcVlan();
    stats->port(port)->arpNotMine();

    updater->receivedArpNotMine(vlan->getID(), senderIP, senderMac, port, op);
    return;
  }

  // This ARP packet is destined to us.
  // Update the sender IP --> sender MAC entry in the ARP table.
  updater->receivedArpMine(vlan->getID(), senderIP, senderMac, port, op);

  // Send a reply if this is an ARP request.
  if (op == ARP_OP_REQUEST) {
    stats->port(port)->arpRequestRx();
    sendArpReply(pkt->getSrcVlan(), pkt->getSrcPort(),
                 entry.value().mac, targetIP,
                 senderMac, senderIP);
    return;
  } else {
    stats->port(port)->arpReplyRx();
    return;
  }

  (void)targetMac; // unused
}

static void sendArp(SwSwitch *sw,
                    VlanID vlan,
                    ArpOpCode op,
                    MacAddress senderMac,
                    IPAddressV4 senderIP,
                    MacAddress targetMac,
                    IPAddressV4 targetIP) {
  VLOG(3) << "sending ARP " << ((op == ARP_OP_REQUEST) ? "request" : "reply")
          << " on vlan " << vlan
          << " to " << targetIP.str() << " (" << targetMac << "): "
          << senderIP.str() << " is " << senderMac;

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

  pkt->writeEthHeader(&cursor, targetMac, senderMac, vlan,
                      ArpHandler::ETHERTYPE_ARP);
  cursor.writeBE<uint16_t>(ARP_HTYPE_ETHERNET);
  cursor.writeBE<uint16_t>(ARP_PTYPE_IPV4);
  cursor.writeBE<uint8_t>(ARP_HLEN_ETHERNET);
  cursor.writeBE<uint8_t>(ARP_PLEN_IPV4);
  cursor.writeBE<uint16_t>(op);
  cursor.push(senderMac.bytes(), MacAddress::SIZE);
  cursor.write<uint32_t>(senderIP.toLong());
  cursor.push(((op == ARP_OP_REQUEST)
               ? MacAddress::ZERO.bytes() : targetMac.bytes()),
              MacAddress::SIZE);
  cursor.write<uint32_t>(targetIP.toLong());
  // Fill the padding with 0s
  memset(cursor.writableData(), 0, cursor.length());

  sw->sendPacketSwitched(std::move(pkt));
}

void ArpHandler::floodGratuituousArp() {
  for (const auto& intf: *sw_->getState()->getInterfaces()) {
    for (const auto& addrEntry: intf->getAddresses()) {
      if (!addrEntry.first.isV4()) {
        continue;
      }
      auto v4Addr = addrEntry.first.asV4();
      // Gratuitous arps have both source and destination IPs set to
      // originator's address
      sendArp(sw_, intf->getVlanID(), ARP_OP_REQUEST, intf->getMac(), v4Addr,
          MacAddress::BROADCAST, v4Addr);
    }
  }

}

void ArpHandler::sendArpReply(VlanID vlan,
                              PortID port,
                              MacAddress senderMac,
                              IPAddressV4 senderIP,
                              MacAddress targetMac,
                              IPAddressV4 targetIP) {
  sw_->stats()->port(port)->arpReplyTx();
  sendArp(sw_, vlan, ARP_OP_REPLY, senderMac, senderIP, targetMac, targetIP);
}

void ArpHandler::sendArpRequest(SwSwitch* sw,
                                const VlanID vlanID,
                                const MacAddress& srcMac,
                                const IPAddressV4& senderIP,
                                const IPAddressV4& targetIP) {
  sw->stats()->arpRequestTx();
  sendArp(sw, vlanID, ARP_OP_REQUEST, srcMac, senderIP,
          MacAddress::BROADCAST, targetIP);
}


void ArpHandler::sendArpRequest(SwSwitch* sw,
                                const shared_ptr<Vlan>& vlan,
                                const IPAddressV4& targetIP) {
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
  auto addrToReach = intf->getAddressToReach(targetIP);

  sendArpRequest(sw, vlan->getID(), intf->getMac(),
                 addrToReach->first.asV4(), targetIP);
}

}} // facebook::fboss
