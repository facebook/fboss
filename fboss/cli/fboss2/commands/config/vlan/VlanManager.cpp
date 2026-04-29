/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/vlan/VlanManager.h"

#include <fmt/format.h>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

const cfg::Vlan* VlanManager::findVlan(
    const cfg::SwitchConfig& swConfig,
    VlanID vlanId) {
  auto it = std::find_if(
      swConfig.vlans()->cbegin(),
      swConfig.vlans()->cend(),
      [vlanId](const auto& vlan) {
        return *vlan.id() == static_cast<int32_t>(vlanId);
      });
  return it != swConfig.vlans()->cend() ? &*it : nullptr;
}

std::pair<bool, cfg::Vlan*> VlanManager::createVlan(
    cfg::SwitchConfig& swConfig,
    VlanID vlanId) {
  // Check if VLAN already exists
  const cfg::Vlan* existingVlan = findVlan(swConfig, vlanId);
  if (existingVlan != nullptr) {
    return {false, const_cast<cfg::Vlan*>(existingVlan)};
  }

  cfg::Vlan newVlan;
  newVlan.id() = static_cast<int32_t>(vlanId);
  newVlan.name() = fmt::format("Vlan{}", static_cast<uint16_t>(vlanId));
  newVlan.routable() = false;

  // Find the right place to insert the new VLAN to maintain sorted order
  // (with the common exception that VLAN 4094 often appears first).
  // Insert the VLAN where the previous VLAN has a smaller ID and the next
  // VLAN has a greater ID. If no such spot is found, append at the end.
  auto insertPos = swConfig.vlans()->end();
  for (auto it = swConfig.vlans()->begin(); it != swConfig.vlans()->end();
       ++it) {
    // Check if we found the right insertion point:
    // The current VLAN has an ID greater than the one we're inserting
    if (*it->id() > static_cast<int32_t>(vlanId)) {
      // Check if the previous VLAN (if exists) has a smaller ID
      if (it == swConfig.vlans()->begin() ||
          *(std::prev(it)->id()) < static_cast<int32_t>(vlanId)) {
        insertPos = it;
        break;
      }
    }
  }

  auto insertedIt = swConfig.vlans()->insert(insertPos, newVlan);

  // Create a corresponding barebone interface — the agent requires every
  // VLAN to have at least one cfg::Interface entry.
  // First check if there's already an interface for this VLAN.
  auto intfIt = std::find_if(
      swConfig.interfaces()->cbegin(),
      swConfig.interfaces()->cend(),
      [vlanId](const auto& intf) {
        return *intf.vlanID() == static_cast<int32_t>(vlanId);
      });

  if (intfIt == swConfig.interfaces()->cend()) {
    // No interface exists for this VLAN yet. We need to create one.
    // Ideally, we use intfID == vlanId, but we need to handle the case where
    // an interface with that ID already exists and belongs to a different VLAN.

    // Find a suitable interface ID
    auto intfId = static_cast<int32_t>(vlanId);
    auto existingIntfIt = std::find_if(
        swConfig.interfaces()->cbegin(),
        swConfig.interfaces()->cend(),
        [intfId](const auto& intf) { return *intf.intfID() == intfId; });

    if (existingIntfIt != swConfig.interfaces()->cend()) {
      // An interface with this ID already exists but belongs to a different
      // VLAN. To avoid future conflicts, we use a separate namespace for
      // conflict resolution: 5000 + vlanId. VLANs are limited to < 4095, so
      // this ensures we don't conflict with normal VLAN IDs.
      intfId = 5000 + static_cast<int32_t>(vlanId);

      // Verify that the conflict-resolution ID is also available
      auto conflictIntfIt = std::find_if(
          swConfig.interfaces()->cbegin(),
          swConfig.interfaces()->cend(),
          [intfId](const auto& intf) { return *intf.intfID() == intfId; });

      if (conflictIntfIt != swConfig.interfaces()->cend()) {
        throw FbossError(
            "Cannot find a suitable interface ID for VLAN ",
            static_cast<uint16_t>(vlanId));
      }
    }

    cfg::Interface newIntf;
    newIntf.intfID() = intfId;
    newIntf.vlanID() = static_cast<int32_t>(vlanId);
    newIntf.name() = fmt::format("fboss{}", static_cast<uint16_t>(vlanId));

    // Find the right place to insert the new interface to maintain sorted order
    // by intfID, similar to how we insert VLANs.
    auto intfInsertPos = swConfig.interfaces()->end();
    for (auto it = swConfig.interfaces()->begin();
         it != swConfig.interfaces()->end();
         ++it) {
      // Check if we found the right insertion point:
      // The current interface has an ID greater than the one we're inserting
      if (*it->intfID() > intfId) {
        // Check if the previous interface (if exists) has a smaller ID
        if (it == swConfig.interfaces()->begin() ||
            *(std::prev(it)->intfID()) < intfId) {
          intfInsertPos = it;
          break;
        }
      }
    }

    swConfig.interfaces()->insert(intfInsertPos, newIntf);
  }

  // Return pointer to the newly inserted VLAN
  return {true, &(*insertedIt)};
}

} // namespace facebook::fboss
