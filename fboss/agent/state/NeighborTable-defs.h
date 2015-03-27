/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/state/NeighborTable.h"

#include <folly/MacAddress.h>
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/NeighborEntry-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

#include <glog/logging.h>

namespace facebook { namespace fboss {

template<typename IPADDR, typename ENTRY, typename SUBCLASS>
NeighborTable<IPADDR, ENTRY, SUBCLASS>::NeighborTable() {
}

template<typename IPADDR, typename ENTRY, typename SUBCLASS>
SUBCLASS* NeighborTable<IPADDR, ENTRY, SUBCLASS>::modify(
    Vlan** vlan, std::shared_ptr<SwitchState>* state) {
  if (!this->isPublished()) {
    CHECK(!(*state)->isPublished());
    return boost::polymorphic_downcast<SUBCLASS*>(this);
  }

  *vlan = (*vlan)->modify(state);
  auto newTable = this->clone();
  auto* ptr = newTable.get();
  (*vlan)->setNeighborTable(std::move(newTable));
  return ptr;
}

template<typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::addEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortID port,
    InterfaceID intfID) {
  CHECK(!this->isPublished());
  auto entry = std::make_shared<Entry>(ip, mac, port, intfID);
  this->addNode(entry);
}

template<typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::updateEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortID port,
    InterfaceID intfID) {
  CHECK(!this->isPublished());
  auto& nodes = this->writableNodes();
  auto it = nodes.find(ip);
  if (it == nodes.end()) {
    throw FbossError("ARP entry for ", ip, " does not exist");
  }
  auto entry = it->second->clone();
  if (entry->isPending()) {
    this->decNPending();
  }
  entry->setMAC(mac);
  entry->setPort(port);
  entry->setIntfID(intfID);
  entry->setPending(false);
  it->second = entry;
}

template<typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::addPendingEntry(
    IPADDR ip,
    InterfaceID intfID) {
  CHECK(!this->isPublished());
  auto pendingEntry = std::make_shared<Entry>(ip, intfID, PENDING);
  this->addNode(pendingEntry);
  this->incNPending();
 }

template<typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::removeEntry(
    AddressType ip) {
  CHECK(!this->isPublished());
  this->removeNode(ip);
}

template<typename IPADDR, typename ENTRY, typename SUBCLASS>
bool NeighborTable<IPADDR, ENTRY, SUBCLASS>::prunePendingEntries() {
  CHECK(!this->isPublished());

  // Need a reverse iterator because removing an element from a flat_map
  // invalidates all iterators after the element removed
  bool modified = false;
  auto entryIt = this->rbegin();
  while (entryIt != this->rend()) {
    auto entry = *entryIt++;
    if (entry->isPending()) {
      VLOG(4) << "Removing pending neighbor entry for " << entry->getIP().str();
      this->removeNode(entry);
      modified = true;
    }
  }
  this->setNPending(0);
  return modified;
}

}} // facebook::fboss
