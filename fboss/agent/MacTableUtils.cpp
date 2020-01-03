/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/MacTableUtils.h"

namespace {

using facebook::fboss::MacEntry;
using facebook::fboss::SwitchState;
using facebook::fboss::VlanID;
using facebook::fboss::cfg::AclLookupClass;

std::shared_ptr<SwitchState> modifyClassIDForEntry(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlanID,
    const MacEntry* macEntry,
    std::optional<AclLookupClass> classID = std::nullopt) {
  auto classIDStr = classID.has_value()
      ? folly::to<std::string>(static_cast<int>(classID.value()))
      : "None";

  auto mac = macEntry->getMac();
  auto portDescr = macEntry->getPort();
  auto vlan = state->getVlans()->getVlanIf(vlanID).get();
  std::shared_ptr<SwitchState> newState{state};
  auto* macTable = vlan->getMacTable().get();
  auto node = macTable->getNodeIf(mac);

  if (node) {
    // Mac Entry is present, associate/disassociate classID
    macTable = macTable->modify(&vlan, &newState);
    macTable->updateEntry(mac, portDescr, classID);
  } else {
    // if MAC is not present, if classID is valid, create an entry and
    // associate it, if the classID is null, do nothing.
    if (classID.has_value()) {
      auto newMacEntry = std::make_shared<MacEntry>(mac, portDescr, classID);
      macTable = macTable->modify(&vlan, &newState);
      macTable->addEntry(newMacEntry);
    }
  }

  return newState;
}
} // namespace

namespace facebook::fboss {

std::shared_ptr<SwitchState> MacTableUtils::updateMacTable(
    const std::shared_ptr<SwitchState>& state,
    L2Entry l2Entry,
    L2EntryUpdateType l2EntryUpdateType) {
  auto vlanID = l2Entry.getVlanID();
  auto mac = l2Entry.getMac();
  auto portDescr = l2Entry.getPort();
  auto vlan = state->getVlans()->getVlanIf(vlanID).get();
  std::shared_ptr<SwitchState> newState{state};
  auto* macTable = vlan->getMacTable().get();
  auto node = macTable->getNodeIf(mac);

  // TODO (skhare) MAC Move handling

  // Delete if the entry to delete exists, otherwise do nothing.
  if (node &&
      l2EntryUpdateType == L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_DELETE) {
    macTable = macTable->modify(&vlan, &newState);
    macTable->removeEntry(mac);
  }

  // Add if the entry to add does not exist, otherwise do nothing.
  if (!node &&
      l2EntryUpdateType == L2EntryUpdateType::L2_ENTRY_UPDATE_TYPE_ADD) {
    auto macEntry = std::make_shared<MacEntry>(mac, portDescr);
    macTable = macTable->modify(&vlan, &newState);
    macTable->addEntry(macEntry);
  }

  return newState;
}

std::shared_ptr<SwitchState> MacTableUtils::updateOrAddEntryWithClassID(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlanID,
    const MacEntry* macEntry,
    cfg::AclLookupClass classID) {
  return modifyClassIDForEntry(state, vlanID, macEntry, classID);
}

std::shared_ptr<SwitchState> MacTableUtils::removeClassIDForEntry(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlanID,
    const MacEntry* macEntry) {
  return modifyClassIDForEntry(state, vlanID, macEntry);
}

} // namespace facebook::fboss
