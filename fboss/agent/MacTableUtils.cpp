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

namespace facebook {
namespace fboss {

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
} // namespace fboss
} // namespace facebook
