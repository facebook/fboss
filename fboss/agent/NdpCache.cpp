/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/NdpCache.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <folly/IPAddressV6.h>

namespace facebook { namespace fboss {

NdpCache::NdpCache(SwSwitch* sw, const SwitchState* state,
                   VlanID vlanID, InterfaceID intfID)
    : NeighborCache<NdpTable>(sw, vlanID, intfID, state->getNdpTimeout(),
                              state->getMaxNeighborProbes(),
                              state->getStaleEntryInterval()) {}

void NdpCache::sentNeighborSolicitation(folly::IPAddressV6 ip) {
  setPendingEntry(ip);
}

void NdpCache::receivedNdpMine(folly::IPAddressV6 ip,
                               folly::MacAddress mac,
                               PortID port,
                               ICMPv6Type type) {
  if (isSolicited(ip)) {
    setEntry(ip, mac, port, NeighborEntryState::REACHABLE);
  } else {
    setEntry(ip, mac, port, NeighborEntryState::STALE);
  }
}

void NdpCache::receivedNdpNotMine(folly::IPAddressV6 ip,
                                  folly::MacAddress mac,
                                  PortID port,
                                  ICMPv6Type type) {
  // Note that ARP updates the forward entry mapping here if necessary.
  // We could potentially do the same here, although the IPv6 NDP RFC
  // doesn't appear to recommend this--it states that we MUST silently
  // discard the packet if the target IP isn't ours.
}

inline void NdpCache::probeFor(folly::IPAddressV6 ip) const {
  auto vlan = getSw()->getState()->getVlans()->getVlanIf(getVlanID());
  if (!vlan) {
    VLOG(2) << "Vlan " << getVlanID() << " not found. Skip sending probe";
    return;
  }
  IPv6Handler::sendNeighborSolicitation(getSw(), ip, vlan);
}

}} // facebook::fboss
