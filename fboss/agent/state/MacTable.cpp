/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/MacTable.h"
#include "fboss/agent/state/NodeMap-defs.h"

#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

MacTable::MacTable() {}

MacTable::~MacTable() {}

MacTable* MacTable::modify(Vlan** vlan, std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto vlans = (*state)->getMultiSwitchVlans()->modify(state);
  auto [node, matcher] = vlans->getNodeAndScope((*vlan)->getID());
  *vlan = (*vlan)->modify(state, matcher);
  auto newMacTable = clone();
  auto* ptr = newMacTable.get();
  (*vlan)->setMacTable(std::move(newMacTable));

  return ptr;
}

MacTable* MacTable::modify(VlanID vlanID, std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto vlanPtr = (*state)->getMultiSwitchVlans()->getNode(vlanID).get();
  return modify(&vlanPtr, state);
}

void MacTable::updateEntry(
    folly::MacAddress mac,
    PortDescriptor portDescr,
    std::optional<cfg::AclLookupClass> classID,
    std::optional<MacEntryType> type) {
  CHECK(!this->isPublished());
  auto entry = getMacIf(mac);
  if (!entry) {
    throw FbossError("Mac entry for ", mac.toString(), " does not exist");
  }
  entry = entry->clone();

  entry->setMac(mac);
  entry->setPort(portDescr);
  entry->setClassID(classID);
  if (type) {
    entry->setType(type.value());
  }
  updateNode(entry);
}

template class ThriftMapNode<MacTable, MacTableTraits>;
} // namespace facebook::fboss
