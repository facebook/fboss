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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

#include <utility>

namespace facebook::fboss {

/**
 * VlanManager provides utilities for managing VLANs in SwitchConfig.
 *
 * This class is used by config vlan subcommands to automatically create VLANs
 * when they do not yet exist, so users do not need to create VLANs explicitly
 * before configuring them.
 */
class VlanManager {
 public:
  // Creates a VLAN in swConfig if it doesn't exist.
  // Returns a pair of (created, vlan_pointer) where:
  //   - created: true if the VLAN was created, false if it already existed
  //   - vlan_pointer: pointer to the VLAN (newly created or existing)
  // Auto-generates a name "Vlan<id>" (e.g., "Vlan100") for new VLANs.
  // Does NOT call saveConfig() — callers save after their own mutations.
  //
  // WARNING: the returned reference is into swConfig.vlans() (a std::vector).
  // Calling push_back / createVlan on the same vector after this call will
  // invalidate the reference.  Use the reference immediately and do not store
  // it across further mutations of swConfig.vlans(), including other calls to
  // this function.
  static std::pair<bool, cfg::Vlan*> createVlan(
      cfg::SwitchConfig& swConfig,
      const VlanID& vlanId);

  // Returns a pointer to the VLAN with the given ID, or nullptr if not found.
  // Use this for read-only lookups where you don't want auto-creation.
  static const cfg::Vlan* findVlan(
      const cfg::SwitchConfig& swConfig,
      const VlanID& vlanId);

  // Removes the VLAN with the given ID from swConfig, along with the barebone
  // L3 interface createVlan() pairs with every VLAN (a vlanID-matching
  // interface that carries no IP addresses) and any static MAC entries scoped
  // to it (child objects that cannot outlive the VLAN).
  //
  // Refuses (throws FbossError) rather than silently orphaning references when
  // the VLAN is still in use. Each referrer must be cleared first:
  //   - it is the global default VLAN (SwitchConfig.defaultVlan)
  //       -> point the default elsewhere: config vlan default <other-id>
  //   - a port lists it as its untagged ingress VLAN (Port.ingressVlan)
  //       -> move the port: config interface <port> switchport access vlan
  //          <other-id>
  //   - a port is a member of it (VlanPort.vlanID)
  //       -> remove membership: config interface <port> switchport trunk
  //          allowed vlan remove <id> (or move the access VLAN as above)
  //   - it backs a routed SVI, i.e. an interface with vlanID == id that has
  //     IP addresses configured
  //       -> remove the addresses: delete interface <name> ip-address /
  //          ipv6-address <addr>
  // Throws FbossError if no VLAN with the given ID exists.
  //
  // Does NOT call saveConfig() — callers save after this returns.
  static void deleteVlan(cfg::SwitchConfig& swConfig, const VlanID& vlanId);
};

} // namespace facebook::fboss
