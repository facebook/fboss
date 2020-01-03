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
#include "fboss/agent/state/NeighborEntry-defs.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"

#include <glog/logging.h>

namespace facebook::fboss {

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
NeighborTable<IPADDR, ENTRY, SUBCLASS>::NeighborTable() {}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
SUBCLASS* NeighborTable<IPADDR, ENTRY, SUBCLASS>::modify(
    Vlan** vlan,
    std::shared_ptr<SwitchState>* state) {
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

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
SUBCLASS* NeighborTable<IPADDR, ENTRY, SUBCLASS>::modify(
    VlanID vlanId,
    std::shared_ptr<SwitchState>* state) {
  if (!this->isPublished()) {
    CHECK(!(*state)->isPublished());
    return boost::polymorphic_downcast<SUBCLASS*>(this);
  }
  // Make clone of table
  auto newTable = this->clone();
  auto* newTablePtr = newTable.get();
  // Make clone of vlan
  auto vlanPtr = (*state)->getVlans()->getVlan(vlanId).get();
  vlanPtr = vlanPtr->modify(state);
  vlanPtr->setNeighborTable(std::move(newTable));
  return newTablePtr;
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::addEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    InterfaceID intfID,
    NeighborState state) {
  CHECK(!this->isPublished());
  auto entry = std::make_shared<Entry>(ip, mac, port, intfID, state);
  this->addNode(entry);
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::addEntry(
    const NeighborEntryFields<AddressType>& fields) {
  addEntry(
      fields.ip, fields.mac, fields.port, fields.interfaceID, fields.state);
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::updateEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    InterfaceID intfID,
    std::optional<cfg::AclLookupClass> classID) {
  CHECK(!this->isPublished());
  auto& nodes = this->writableNodes();
  auto it = nodes.find(ip);
  if (it == nodes.end()) {
    throw FbossError("Neighbor entry for ", ip, " does not exist");
  }
  auto entry = it->second->clone();
  entry->setMAC(mac);
  entry->setPort(port);
  entry->setIntfID(intfID);
  entry->setState(NeighborState::REACHABLE);
  entry->setClassID(classID);
  it->second = entry;
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::updateEntry(
    AddressType ip,
    std::shared_ptr<ENTRY> newEntry) {
  auto& nodes = this->writableNodes();
  auto it = nodes.find(ip);
  if (it == nodes.end()) {
    throw FbossError("Neighbor entry for ", ip, " does not exist");
  }
  it->second = newEntry;
  return;
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::updateEntry(
    const NeighborEntryFields<AddressType>& fields) {
  updateEntry(
      fields.ip, fields.mac, fields.port, fields.interfaceID, fields.classID);
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::addPendingEntry(
    IPADDR ip,
    InterfaceID intfID) {
  CHECK(!this->isPublished());
  auto pendingEntry =
      std::make_shared<Entry>(ip, intfID, NeighborState::PENDING);
  this->addNode(pendingEntry);
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::removeEntry(AddressType ip) {
  CHECK(!this->isPublished());
  this->removeNode(ip);
}

} // namespace facebook::fboss
