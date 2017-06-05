/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "UnresolvedNhopsProber.h"
#include "fboss/agent/NexthopToRouteCount.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/IPv6Handler.h"

namespace facebook { namespace fboss {

void UnresolvedNhopsProber::timeoutExpired() noexcept {
  std::lock_guard<std::mutex> g(lock_);
  auto state = sw_->getState();
  for (const auto& ridAndNhopsRefCounts : nhops2RouteCount_) {
    for (const auto& nhopAndRefCount : ridAndNhopsRefCounts.second) {
      const auto& nhop = nhopAndRefCount.first;
      auto intf = state->getInterfaces()->getInterfaceIf(nhop.intf());
      if (!intf) {
        continue; // interface got unconfigured
      }
      // Probe all nexthops for which either don't have a L2 entry
      // or the entry is not resolved (port == 0). Note that we do
      // not exclude pending entries here since in case of recursive
      // routes we might get packets with destination set to prefix
      // that needs to be resolved recursively. In ARP and NDP code
      // we do not do route lookup when deciding to send ARP/NDP requests.
      // So we would only try to ARP/NDP for the destination if it
      // is in one of the interface subnets (which it won't be else
      // we won't have needed recursive resolution). So ARP/NDP for
      // all unresolved next hops. We could also consider doing route
      // lookups in ARP/NDP code, but by probing all unresolved next
      // hops we effectively do the same thing, since the next hops
      // probed come from after the route was (recursively) resolved.
      auto vlan = state->getVlans()->getVlanIf(intf->getVlanID());
      CHECK(vlan); // must have vlan for configrued inteface
      if (nhop.addr().isV4()) {
        auto nhop4 = nhop.addr().asV4();
        auto arpEntry = vlan->getArpTable()->getEntryIf(nhop4);
        if (!arpEntry || arpEntry->getPort() == PortID(0)) {
          VLOG(3) <<" Sending probe for unresolved next hop: " << nhop4;
          ArpHandler::sendArpRequest(sw_, vlan, nhop4);
        }
      } else {
        auto nhop6 = nhop.addr().asV6();
        auto ndpEntry = vlan->getNdpTable()->getEntryIf(nhop6);
        if (!ndpEntry || ndpEntry->getPort() == PortID(0)) {
          VLOG(3) <<" Sending probe for unresolved next hop: " << nhop6;
          IPv6Handler::sendNeighborSolicitation(sw_, nhop6, vlan);
        }
      }
    }
  }
  scheduleTimeout(interval_);
}

}} // facebook::fboss
