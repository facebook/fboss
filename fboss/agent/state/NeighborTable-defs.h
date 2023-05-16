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
SUBCLASS* NeighborTable<IPADDR, ENTRY, SUBCLASS>::modify(
    Interface** interface,
    std::shared_ptr<SwitchState>* state) {
  if (!this->isPublished()) {
    CHECK(!(*state)->isPublished());
    return boost::polymorphic_downcast<SUBCLASS*>(this);
  }

  *interface = (*interface)->modify(state);
  auto newTable = this->clone();
  (*interface)->setNeighborEntryTable<IPADDR>(newTable->toThrift());
  return (*interface)->getNeighborEntryTable<IPADDR>().get();
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
SUBCLASS* NeighborTable<IPADDR, ENTRY, SUBCLASS>::modify(
    InterfaceID interfaceId,
    std::shared_ptr<SwitchState>* state) {
  if (!this->isPublished()) {
    CHECK(!(*state)->isPublished());
    return boost::polymorphic_downcast<SUBCLASS*>(this);
  }
  // Make clone of table
  auto newTable = this->clone();
  // Make clone of interface
  auto interfacePtr = (*state)->getInterfaces()->getNode(interfaceId).get();
  interfacePtr = interfacePtr->modify(state);
  interfacePtr->setNeighborEntryTable<IPADDR>(newTable->toThrift());
  return interfacePtr->getNeighborEntryTable<IPADDR>().get();
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::addEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    InterfaceID intfID,
    NeighborState state,
    std::optional<cfg::AclLookupClass> classID,
    std::optional<int64_t> encapIndex,
    bool isLocal) {
  CHECK(!this->isPublished());
  state::NeighborEntryFields thrift{};
  thrift.ipaddress() = ip.str();
  thrift.mac() = mac.toString();
  thrift.portId() = port.toThrift();
  thrift.interfaceId() = intfID;
  thrift.state() = static_cast<state::NeighborState>(state);
  if (classID) {
    thrift.classID() = *classID;
  }
  if (encapIndex) {
    thrift.encapIndex() = *encapIndex;
  }
  thrift.isLocal() = isLocal;
  auto entry = std::make_shared<Entry>(std::move(thrift));
  this->addNode(entry);
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::addEntry(
    const NeighborEntryFields<AddressType>& fields) {
  addEntry(
      fields.ip,
      fields.mac,
      fields.port,
      fields.interfaceID,
      fields.state,
      fields.classID,
      fields.encapIndex,
      fields.isLocal);
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::updateEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    InterfaceID intfID,
    NeighborState state,
    std::optional<cfg::AclLookupClass> classID,
    std::optional<int64_t> encapIndex,
    bool isLocal) {
  auto entry = this->getNode(ip.str());
  entry = entry->clone();
  entry->setMAC(mac);
  entry->setPort(port);
  entry->setIntfID(intfID);
  entry->setState(state);
  entry->setClassID(classID);
  entry->setEncapIndex(encapIndex);
  entry->setIsLocal(isLocal);
  this->updateNode(std::move(entry));
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::updateEntry(
    AddressType /*ip*/,
    std::shared_ptr<ENTRY> newEntry) {
  this->updateNode(std::move(newEntry));
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::updateEntry(
    const NeighborEntryFields<AddressType>& fields) {
  updateEntry(
      fields.ip,
      fields.mac,
      fields.port,
      fields.interfaceID,
      fields.state,
      fields.classID,
      fields.encapIndex,
      fields.isLocal);
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::addPendingEntry(
    IPADDR ip,
    InterfaceID intfID) {
  CHECK(!this->isPublished());
  state::NeighborEntryFields thrift{};
  thrift.ipaddress() = ip.str();
  thrift.interfaceId() = intfID;
  thrift.state() = state::NeighborState::Pending;
  auto pendingEntry = std::make_shared<Entry>(std::move(thrift));
  this->addNode(pendingEntry);
}

template <typename IPADDR, typename ENTRY, typename SUBCLASS>
void NeighborTable<IPADDR, ENTRY, SUBCLASS>::removeEntry(AddressType ip) {
  CHECK(!this->isPublished());
  this->removeNode(ip.str());
}

} // namespace facebook::fboss
