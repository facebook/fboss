/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ArpCache.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/types.h"

#include <folly/IPAddressV4.h>
#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

ArpCache::ArpCache(
    SwSwitch* sw,
    const SwitchState* state,
    VlanID vlanID,
    std::string vlanName,
    InterfaceID intfID)
    : NeighborCache<ArpTable>(
          sw,
          vlanID,
          vlanName,
          intfID,
          state->getArpTimeout(),
          state->getMaxNeighborProbes(),
          state->getStaleEntryInterval()) {}

void ArpCache::sentArpRequest(folly::IPAddressV4 ip) {
  // Pending entry points to CPU port
  setPendingEntry(ip, PortDescriptor(PortID(0)));
}

void ArpCache::receivedArpMine(
    folly::IPAddressV4 ip,
    folly::MacAddress mac,
    PortDescriptor port,
    ArpOpCode /*op*/) {
  // always set an entry, even if the reply was unsolicited
  setEntry(ip, mac, port, NeighborEntryState::REACHABLE);
}

void ArpCache::receivedArpNotMine(
    folly::IPAddressV4 ip,
    folly::MacAddress mac,
    PortDescriptor port,
    ArpOpCode /*op*/) {
  // Update the sender IP --> sender MAC entry in our ARP table
  // only if it already exists.
  // (This behavior follows RFC 826.)
  setExistingEntry(ip, mac, port, NeighborEntryState::REACHABLE);
}

inline void ArpCache::checkReachability(
    folly::IPAddressV4 targetIP,
    folly::MacAddress /*targetMac*/,
    PortDescriptor /*port*/) const {
  return probeFor(targetIP);
}

inline void ArpCache::probeFor(folly::IPAddressV4 ip) const {
  auto vlan = getSw()->getState()->getVlans()->getVlanIf(getVlanID());
  if (!vlan) {
    XLOG(DBG2) << "Vlan " << getVlanID() << " not found. Skip sending probe";
    return;
  }
  ArpHandler::sendArpRequest(getSw(), vlan, ip);
}

std::list<ArpEntryThrift> ArpCache::getArpCacheData() {
  return getCacheData<ArpEntryThrift>();
}

} // namespace facebook::fboss
