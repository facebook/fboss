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

#include <map>
#include <optional>
#include <set>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class PlatformMapping;
namespace utility {

/*
 * Utilities for generating and editing port-related portions of an agent
 * SwitchConfig. This library is shared by FBOSS hardware tests and the fboss2
 * config CLI so both paths use the same configuration logic.
 */

/*
 * Use vlan 2000, as the base vlan for ports in configs generated here.
 * Anything except 0, 1 would actually work fine. 0 because
 * its reserved, and 1 because BRCM uses that as default VLAN.
 * So for example if we use VLAN 1, BRCM will also add cpu port to
 * that vlan along with our configured ports. This causes unnecessary
 * confusion for our tests.
 */
auto constexpr kBaseVlanId = 2000;
/*
 * Default VLAN
 */
auto constexpr kDefaultVlanId4094 = 4094;
auto constexpr kDefaultVlanId1 = 1;
auto constexpr kDownlinkBaseVlanId = 2000;
auto constexpr kUplinkBaseVlanId = 4000;

// Per-port interface-vlan allocation band. INTERFACE_PORTs allocate from the
// low end while MANAGEMENT_PORTs allocate from the high end. The band starts
// just above the shared downlink base (kBaseVlanId) and stops before the
// sidelink band (3000), so allocated ids avoid every reserved range (loopback
// 10/11, sidelink 3000-3024, uplink 3100/4001+, default 4094).
auto constexpr kInterfaceVlanIdMin = kBaseVlanId + 1;
auto constexpr kInterfaceVlanIdMax = 2999;

struct SafeProfileSelectionOptions {
  cfg::AsicType asicType{cfg::AsicType::ASIC_TYPE_FAKE};
  // When true, ports outside requiredPorts may be omitted or subsumed.
  bool supportsAddRemovePort{false};
  bool dualStageRdsw3q2q{false};
  bool preferOpticalProfiles{false};
  // Ports which must coexist in the resulting configuration. When unset,
  // every port in each supplied group is required.
  std::optional<std::set<PortID>> requiredPorts;
};

using PortProfileMap = std::map<PortID, cfg::PortProfileID>;

// Selects one compatible profile per controlling-port group and returns that
// profile for every port which must be present in the resulting config. The
// fastest safe profile wins; when requested, equal-speed interface and
// management profiles prefer optical when available.
PortProfileMap getSafeProfileIDs(
    const PlatformMapping& platformMapping,
    const std::map<PortID, std::vector<PortID>>&
        controllingPortToSubsidiaryPorts,
    const SafeProfileSelectionOptions& options);

// Bare port body. Speed derived from PlatformMapping (nullopt ->
// cfg::PortSpeed::DEFAULT). Sets name/portType/scope (from
// mapping->getPlatformPort(id).mapping()), logicalID=id, profileID,
// state=DISABLED, speed, ingressVlan. Does NOT set routable.
cfg::Port createDefaultPortConfig(
    const PlatformMapping* platformMapping,
    PortID id,
    cfg::PortProfileID profileID,
    int32_t ingressVlan);

// Basic building blocks for a routed NPU interface or management port. Callers
// may adjust policy-specific fields before inserting the returned objects into
// a config.
cfg::Port createRoutedPortConfig(
    const PlatformMapping& platformMapping,
    PortID id,
    cfg::PortProfileID profileID,
    VlanID ingressVlan);

// As above, but restricted to INTERFACE_PORT.
cfg::Port createInterfacePortConfig(
    const PlatformMapping& platformMapping,
    PortID id,
    cfg::PortProfileID profileID,
    VlanID ingressVlan);
cfg::Vlan createVlanConfig(VlanID id);
cfg::VlanPort createVlanPortConfig(PortID portID, VlanID vlanID);
cfg::Interface createVlanInterfaceConfig(
    InterfaceID interfaceID,
    VlanID vlanID);

// Lowest-free vlan id in [minId, maxId] not used by any vlan id or any
// interface intfID in config. Throws FbossError if none free.
int32_t allocateFreeVlanId(
    const cfg::SwitchConfig& config,
    int32_t minId = kInterfaceVlanIdMin,
    int32_t maxId = kInterfaceVlanIdMax);

// Append a routed VLAN-style interface or management port into an existing
// config, using vlanID for both the VLAN and interface ID.
void addRoutedPortToConfig(
    cfg::SwitchConfig& config,
    const PlatformMapping* platformMapping,
    PortID id,
    cfg::PortProfileID profileID,
    VlanID vlanID);

// Append a VLAN-style interface port into an existing config, using vlanID for
// both the VLAN and interface ID.
void addInterfacePortToConfig(
    cfg::SwitchConfig& config,
    const PlatformMapping* platformMapping,
    PortID id,
    cfg::PortProfileID profileID,
    VlanID vlanID);

// As above, but allocate the lowest free VLAN/interface ID and return it.
int32_t addInterfacePortToConfig(
    cfg::SwitchConfig& config,
    const PlatformMapping* platformMapping,
    PortID id,
    cfg::PortProfileID profileID);

// How a port is removed from the config.
enum class PortRemovalMode {
  // Drop the port entry (and its vlanPort).
  Erase,
  // Keep the port entry but set its state to DISABLED. Used by platforms that
  // do not support adding/removing ports at runtime.
  Disable,
};

// Removes `portsToRemove` from `config`. In Erase mode each port and its
// vlanPort are dropped; in Disable mode each port is kept but set to DISABLED
// (vlanPorts/vlans/interfaces untouched). When `pruneEmptyVlansAndInterfaces`
// is set (Erase mode only), vlans left with no member vlanPort are dropped
// along with the VLAN interface on each (originally-empty and default vlans are
// preserved). Vlan membership is judged by vlanPorts, so tagged/multi-vlan
// members are handled and no vlanPort is left pointing at a dropped vlan.
// VLAN-style configs only: port-router (PORT-type) interfaces, which bind to a
// port via portID rather than a vlan, are left untouched for now (TODO: prune
// them once platform support is verified).
void removePortsFromConfig(
    cfg::SwitchConfig& config,
    const std::set<PortID>& portsToRemove,
    PortRemovalMode mode = PortRemovalMode::Erase,
    bool pruneEmptyVlansAndInterfaces = true);

} // namespace utility
} // namespace facebook::fboss
