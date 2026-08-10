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

#include <set>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

/**
 * InterfaceManager provides utilities for managing L3 router interfaces
 * (SwitchConfig.interfaces) that are not tied to a single port, such as VLAN
 * SVIs and virtual/loopback interfaces.
 *
 * Port deletion prunes the interfaces it owns via
 * utility::removePortsFromConfig; this class covers the interfaces that
 * outlive any one port.
 */
class InterfaceManager {
 public:
  // Removes the interfaces with the given IDs from swConfig, along with the
  // VLAN intfID back-pointers naming them.
  //
  // Every ID is checked before any of them is removed, so a refused delete
  // leaves the config untouched rather than partially applied. Checking the
  // set as a whole also means two interfaces sharing a VLAN can be deleted
  // together: neither counts as the other's surviving cover.
  //
  // Refuses (throws FbossError) rather than leaving a dangling reference or a
  // config the agent will reject — or crash on — at apply time:
  //   - the interface is a port router interface (InterfaceType::PORT).
  //     Deleting it leaves its port with an empty interface list, and
  //     Port::getInterfaceID() CHECK-fails on that, taking the agent down on
  //     the first packet routed via the port
  //       -> delete the port instead: delete interface <port-name>
  //   - an ip-in-ip or SRv6 tunnel uses it as its underlay interface
  //     (Tunnel.underlayIntfID is a required field, so there is nothing to
  //     clear)
  //       -> delete the tunnel first: delete tunnel <id>
  //   - it is the last VLAN-type interface for its VLAN and that VLAN still
  //     has an enabled member port; ThriftConfigApplier rejects such a config
  //     with "VLAN <id> has no interface, even when corresp port <port> is
  //     enabled"
  //       -> disable or unbind the member ports, or drop the whole VLAN with
  //          delete vlan <id>
  //     Ports listed in portsBeingDeleted are excluded from this check: a
  //     single 'delete interface <svi> <its-only-port>' removes the port too,
  //     so the VLAN is not left with a live port and no interface.
  // Throws FbossError if any of the given interfaces does not exist.
  //
  // An ACL redirect-nexthop naming one of these interfaces
  // (RedirectNextHop.intfID) is deliberately not a refusal: the field is
  // optional and the agent does not resolve it against the interface list, it
  // just disables the ACL when no nexthop resolves.
  //
  // Does NOT call saveConfig() — callers save after this returns.
  static void deleteInterfaces(
      cfg::SwitchConfig& swConfig,
      const std::set<InterfaceID>& intfIds,
      const std::set<PortID>& portsBeingDeleted = {});
};

} // namespace facebook::fboss
