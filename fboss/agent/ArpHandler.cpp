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

enum ArpOpCode : uint16_t {
  ARP_OP_REQUEST = 1,
  ARP_OP_REPLY = 2,
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
  const uint32_t l2Len = pkt->getLength();
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
 
  if (sw_->runningInNetlinkMode())
  {
    if (sw_->sendPacketToHost(std::move(pkt)))
    {
      stats->port(port)->pktToHost(l2Len);
    }
    else
    {
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
  auto op = cursor.readBE<uint16_t>();
  auto senderMac = PktUtil::readMac(&cursor);
  auto senderIP = PktUtil::readIPv4(&cursor);
  auto targetMac = PktUtil::readMac(&cursor);
  auto targetIP = PktUtil::readIPv4(&cursor);

  // Check to see if this IP address is in our ARP response table.
  auto entry = vlan->getArpResponseTable()->getEntry(targetIP);
  if (!entry) {
    // The target IP does not refer to us.
    VLOG(5) << "ignoring ARP message for " << targetIP.str()
            << " on vlan " << pkt->getSrcVlan();
    stats->port(port)->arpNotMine();
    // Update the sender IP --> sender MAC entry in our ARP table
    // only if it already exists.
    // (This behavior follows RFC 826.)
    updateExistingArpEntry(vlan, senderIP, senderMac, pkt->getSrcPort());
    return;
  }

  // This ARP packet is destined to us.
  // Update the sender IP --> sender MAC entry in the ARP table.
  updateArpEntry(vlan, senderIP, senderMac, pkt->getSrcPort());

  // Send a reply if this is an ARP request.
  if (op == ARP_OP_REQUEST) {
    stats->port(port)->arpRequestRx();
    sendArpReply(pkt->getSrcVlan(), pkt->getSrcPort(),
                 entry.value().mac, targetIP,
                 senderMac, senderIP);
    return;
  } else if (op == ARP_OP_REPLY) {
    stats->port(port)->arpReplyRx();
    return;
  } else {
    stats->port(port)->arpBadOp();
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

uint32_t ArpHandler::flushArpEntryBlocking(IPAddressV4 ip, VlanID vlan) {
  uint32_t count{0};
  auto updateFn = [&](const std::shared_ptr<SwitchState>& state) {
    return performNeighborFlush(state, vlan, ip, &count);
  };
  sw_->updateStateBlocking("flush ARP entry", updateFn);
  return count;
}

std::shared_ptr<SwitchState> ArpHandler::performNeighborFlush(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlanID,
    folly::IPAddressV4 ip,
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

bool ArpHandler::performNeighborFlush(std::shared_ptr<SwitchState>* state,
                                      Vlan* vlan,
                                      folly::IPAddressV4 ip) {
  auto* arpTable = vlan->getArpTable().get();
  const auto& entry = arpTable->getNodeIf(ip);
  if (!entry) {
    return false;
  }

  arpTable = arpTable->modify(&vlan, state);
  arpTable->removeNode(ip);
  return true;
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

void ArpHandler::sendArpRequest(shared_ptr<Vlan> vlan,
                                IPAddressV4 senderIP,
                                IPAddressV4 targetIP) {
  sw_->stats()->arpRequestTx();
  auto intf = sw_->getState()->getInterfaces()
              ->getInterfaceInVlan(vlan->getID());

  sendArp(sw_, vlan->getID(), ARP_OP_REQUEST, intf->getMac(), senderIP,
          MacAddress::BROADCAST, targetIP);
  setPendingArpEntry(vlan, targetIP);
}

void ArpHandler::updateExistingArpEntry(const shared_ptr<Vlan>& origVlan,
                                        IPAddressV4 ip,
                                        MacAddress mac,
                                        PortID port) {
  // Check to see if we already have an entry.  We only need to update
  // the entry if it already exists, but is out of date.
  auto arpTable = origVlan->getArpTable();
  auto entry = arpTable->getNodeIf(ip);
  if (!entry) {
    // This IP isn't already in the map, and we aren't supposed to add it if
    // it isn't present already.
    return;
  }
  if (entry->getMac() == mac &&
      entry->getPort() == port &&
      !entry->isPending()) {
    // The entry is up-to-date.
    return;
  }

  arpUpdateRequired(origVlan->getID(), ip, mac, port,
                    entry->getIntfID(), false);
}

void ArpHandler::setPendingArpEntry(shared_ptr<Vlan> vlan,
                                    IPAddressV4 ip) {
  auto vlanID = vlan->getID();
  auto intfID = vlan->getInterfaceID();
  VLOG(4) << "setting pending entry on vlan " << vlanID << " with ip "
          << ip.str() << " and intf " << intfID;

  auto arpTable = vlan->getArpTable();
  auto entry = arpTable->getNodeIf(ip);
  if (entry) {
    // Don't overwrite any entry with a pending entry
    return;
  }

  auto updateFn = [=](const shared_ptr<SwitchState>& state)
    -> shared_ptr<SwitchState> {
    auto vlans = state->getVlans();
    auto vlan1 = vlans->getVlanIf(vlanID);
    auto* newVlan = vlan1.get();
    if (!newVlan) {
      // This VLAN no longer exists.  Just ignore the ARP entry update.
      VLOG(4) << "VLAN " << vlanID <<
        " deleted before pending ARP entry could be added";
      return nullptr;
    }

    auto intfs = state->getInterfaces();
    auto intf = intfs->getInterfaceIf(intfID);
    if (!intf) {
      VLOG(4) << "Interface " << intfID <<
        " deleted before pending ARP entry could be added";
      return nullptr;
    }
    if (!intf->canReachAddress(ip)) {
      VLOG(4) << ip << " deleted from interface " << intfID <<
        " before pending ARP entry could be added";
      return nullptr;
    }

    shared_ptr<SwitchState> newState{state};
    auto* arpTable = newVlan->getArpTable().get();
    auto entry = arpTable->getNodeIf(ip);
    if (!entry) {
      // only set a pending entry when we have no entry
      arpTable = arpTable->modify(&newVlan, &newState);
      arpTable->addPendingEntry(ip, intfID);
    }

    VLOG(4) << "Adding pending ARP entry for " << ip.str() <<
      " on interface " << intfID;
    return newState;
  };

  sw_->updateState("add pending ARP entry", updateFn);
}

void ArpHandler::updateArpEntry(const shared_ptr<Vlan>& origVlan,
                                IPAddressV4 ip,
                                MacAddress mac,
                                PortID port) {
  // Perform an initial check to see if we need to do anything.
  // In the common case the entry will already be up-to-date and we don't need
  // to do anything.
  auto arpTable = origVlan->getArpTable();
  auto entry = arpTable->getNodeIf(ip);
  auto intfID = origVlan->getInterfaceID();

  if (entry &&
      entry->getMac() == mac &&
      entry->getPort() == port &&
      entry->getIntfID() == intfID &&
      !entry->isPending()) {
    // The entry is up-to-date, so we have nothing to do.
    return;
  }

  // We really need to grab the lock and perform an update
  arpUpdateRequired(origVlan->getID(), ip, mac, port, intfID, true);
}

void ArpHandler::arpUpdateRequired(VlanID vlanID,
                                   IPAddressV4 ip,
                                   MacAddress mac,
                                   PortID port,
                                   InterfaceID intfID,
                                   bool addNewEntry) {
  // Ignore the entry if the IP isn't locally reachable on this interface
  if (!Interface::isIpAttached(ip, intfID, sw_->getState())) {
    LOG(WARNING) << "Skip updating un-reachable ARP entry " << ip << " --> "
      << mac << " on interface " << intfID;
    return;
  }

  auto updateFn = [=](const shared_ptr<SwitchState>& state)
      -> shared_ptr<SwitchState> {
    auto* vlan = state->getVlans()->getVlanIf(vlanID).get();
    if (!vlan) {
      // This VLAN no longer exists.  Just ignore the ARP entry update.
      VLOG(1) << "VLAN " << vlanID << " deleted before ARP entry " <<
        ip << " --> " << mac << " could be updated";
      return nullptr;
    }

    // The interfaces may be different now, so re-verify the address
    if (!Interface::isIpAttached(ip, intfID, state)) {
      LOG(WARNING) << "ARP entry " << ip << " --> "
        << mac << " on interface " << intfID << " became unreachable before "
        "it could be updated";
      return nullptr;
    }

    shared_ptr<SwitchState> newState{state};
    auto* arpTable = vlan->getArpTable().get();
    auto entry = arpTable->getNodeIf(ip);
    if (!entry) {
      if (!addNewEntry) {
        // This entry was deleted while we were waiting on the lock, and we
        // aren't supposed to re-add it.
        return nullptr;
      }
      arpTable = arpTable->modify(&vlan, &newState);
      arpTable->addEntry(ip, mac, port, intfID);
    } else {
      if (entry->getMac() == mac &&
          entry->getPort() == port &&
          entry->getIntfID() == intfID &&
          !entry->isPending()) {
        // This entry was already updated while we were waiting on the lock.
        return nullptr;
      }
      arpTable = arpTable->modify(&vlan, &newState);
      arpTable->updateEntry(ip, mac, port, intfID);
    }
    VLOG(3) << "Adding ARP entry for " << ip.str() << " --> " << mac;
    return newState;
  };
  sw_->updateState("add ARP entry", updateFn);
}

}} // facebook::fboss
