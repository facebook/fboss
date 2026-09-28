/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/config/agent/PortConfigUtils.h"

#include <algorithm>
#include <set>
#include <string>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss::utility {
namespace {

std::optional<cfg::PortSpeed> getProfileSpeed(
    const PlatformMapping& platformMapping,
    cfg::PortProfileID profileID,
    const std::vector<PortID>& ports) {
  for (const auto& portID : ports) {
    const auto& supportedProfiles =
        *platformMapping.getPlatformPort(portID).supportedProfiles();
    if (supportedProfiles.find(profileID) == supportedProfiles.end()) {
      continue;
    }
    if (const auto profileConfig = platformMapping.getPortProfileConfig(
            PlatformPortProfileConfigMatcher(profileID, portID))) {
      return *profileConfig->speed();
    }
  }
  return std::nullopt;
}

bool isRequiredPort(
    PortID portID,
    const std::optional<std::set<PortID>>& requiredPorts) {
  return !requiredPorts || requiredPorts->find(portID) != requiredPorts->end();
}

} // namespace

PortProfileMap getSafeProfileIDs(
    const PlatformMapping& platformMapping,
    const std::map<PortID, std::vector<PortID>>&
        controllingPortToSubsidiaryPorts,
    const SafeProfileSelectionOptions& options) {
  // For each controlling-port group, this function:
  // 1. Collects profiles that do not subsume another required port.
  // 2. Applies any ASIC-specific fixed speed or profile requirement.
  // 3. Otherwise selects the fastest safe profile.
  // 4. Assigns that profile to every port which must remain in the config.
  PortProfileMap portToProfileIDs;
  const auto& platformEntries = platformMapping.getPlatformPorts();
  for (const auto& [controllingPort, ports] :
       controllingPortToSubsidiaryPorts) {
    // Find the safe profiles that can satisfy all required ports in the group.
    std::set<cfg::PortProfileID> safeProfiles;
    for (const auto& portID : ports) {
      const auto portEntry = platformEntries.find(portID);
      if (portEntry == platformEntries.end()) {
        throw FbossError("Port ", portID, " does not exist in PlatformMapping");
      }
      for (const auto& [profileID, profile] :
           *portEntry->second.supportedProfiles()) {
        auto subsumedPorts = profile.subsumedPorts();
        // A higher-speed profile is safe when its subsumed ports do not
        // overlap the group, or when the platform supports adding/removing
        // ports and none of the subsumed ports are required.
        const auto conflictsWithRequiredPort = subsumedPorts &&
            std::any_of(subsumedPorts->begin(),
                        subsumedPorts->end(),
                        [&](const auto subsumedPort) {
                          const auto port = PortID(subsumedPort);
                          return std::find(ports.begin(), ports.end(), port) !=
                              ports.end() &&
                              (!options.supportsAddRemovePort ||
                               isRequiredPort(port, options.requiredPorts));
                        });
        if (!conflictsWithRequiredPort) {
          // Profiles without subsumed ports are inherently safe; profiles
          // with subsumed ports reach here only when no required port overlaps.
          safeProfiles.insert(profileID);
        }
      }
    }

    if (safeProfiles.empty()) {
      std::string portSet;
      for (const auto& portID : ports) {
        if (!portSet.empty()) {
          portSet += ", ";
        }
        portSet += std::to_string(static_cast<int32_t>(portID));
      }
      throw FbossError("Can't find safe profiles for ports: ", portSet);
    }

    auto bestSpeed = cfg::PortSpeed::DEFAULT;
    auto bestProfile = cfg::PortProfileID::PROFILE_DEFAULT;
    const auto controllingPortEntry = platformEntries.find(controllingPort);
    if (controllingPortEntry == platformEntries.end()) {
      throw FbossError(
          "Controlling port ",
          controllingPort,
          " does not exist in PlatformMapping");
    }
    const auto portType = *controllingPortEntry->second.mapping()->portType();
    if ((options.asicType == cfg::AsicType::ASIC_TYPE_JERICHO3 ||
         options.asicType == cfg::AsicType::ASIC_TYPE_JERICHO4) &&
        options.dualStageRdsw3q2q &&
        portType == cfg::PortType::INTERFACE_PORT) {
      // The dual-stage RDSW 3Q2Q chip config uses 400G NIF ports, and Jericho3
      // does not yet support changing port speed dynamically.
      bestSpeed = cfg::PortSpeed::FOURHUNDREDG;
    } else if (
        options.asicType == cfg::AsicType::ASIC_TYPE_CHENAB &&
        portType == cfg::PortType::INTERFACE_PORT) {
      // Chenab production configs use this 400G profile. Changing profiles
      // may recreate ports through delete/add, which Chenab does not support.
      // Minipack3N also has a maximum port speed of 400G.
      bestSpeed = cfg::PortSpeed::FOURHUNDREDG;
      bestProfile = cfg::PortProfileID::PROFILE_400G_4_PAM4_RS544X2N_OPTICAL;
    }

    // Without a fixed speed requirement, select the fastest safe profile.
    const auto pickMaxSpeed = bestSpeed == cfg::PortSpeed::DEFAULT;
    if (bestProfile == cfg::PortProfileID::PROFILE_DEFAULT) {
      for (const auto profileID : safeProfiles) {
        const auto speed = getProfileSpeed(platformMapping, profileID, ports);
        if (!speed) {
          throw FbossError(
              "Can't resolve speed for profile ",
              profileID,
              " in controlling-port group ",
              controllingPort);
        }
        if ((pickMaxSpeed &&
             static_cast<int>(bestSpeed) < static_cast<int>(*speed)) ||
            (!pickMaxSpeed && *speed == bestSpeed)) {
          bestSpeed = *speed;
          bestProfile = profileID;
        }
      }
    } else if (
        safeProfiles.find(bestProfile) == safeProfiles.end() ||
        getProfileSpeed(platformMapping, bestProfile, ports) != bestSpeed) {
      throw FbossError(
          "Profile ",
          bestProfile,
          " is not a safe profile at speed ",
          bestSpeed,
          " for controlling-port group ",
          controllingPort);
    }
    if (bestProfile == cfg::PortProfileID::PROFILE_DEFAULT) {
      throw FbossError(
          "Can't find a safe profile at speed ",
          bestSpeed,
          " for controlling-port group ",
          controllingPort);
    }

    // Add/remove platforms may omit ports outside the required port set.
    for (const auto& portID : ports) {
      if (options.supportsAddRemovePort && options.requiredPorts &&
          !isRequiredPort(portID, options.requiredPorts)) {
        continue;
      }
      portToProfileIDs.emplace(portID, bestProfile);
    }
  }
  return portToProfileIDs;
}

cfg::Port createDefaultPortConfig(
    const PlatformMapping* platformMapping,
    PortID id,
    cfg::PortProfileID profileID,
    int32_t ingressVlan) {
  if (!platformMapping) {
    throw FbossError("Platform mapping must not be null");
  }
  cfg::Port port;
  const auto& mapping = *platformMapping->getPlatformPort(id).mapping();
  port.name() = *mapping.name();
  port.portType() = *mapping.portType();
  port.scope() = *mapping.scope();
  port.logicalID() = id;
  port.profileID() = profileID;
  port.state() = cfg::PortState::DISABLED;
  port.ingressVlan() = ingressVlan;

  const auto profileConfig = platformMapping->getPortProfileConfig(
      PlatformPortProfileConfigMatcher(profileID, id));
  port.speed() = profileConfig.has_value() ? *profileConfig->speed()
                                           : cfg::PortSpeed::DEFAULT;
  return port;
}

int32_t allocateFreeVlanId(
    const cfg::SwitchConfig& config,
    int32_t minId,
    int32_t maxId) {
  std::set<int32_t> usedIds;
  for (const auto& vlan : *config.vlans()) {
    usedIds.insert(*vlan.id());
  }
  for (const auto& intf : *config.interfaces()) {
    usedIds.insert(*intf.intfID());
  }
  for (int32_t candidate = minId; candidate <= maxId; ++candidate) {
    if (usedIds.find(candidate) == usedIds.end()) {
      return candidate;
    }
  }
  throw FbossError(
      "No free vlan id available in range [", minId, ", ", maxId, "]");
}

int32_t addInterfacePortToConfig(
    cfg::SwitchConfig& config,
    const PlatformMapping* platformMapping,
    PortID id,
    cfg::PortProfileID profileID) {
  const int32_t n = allocateFreeVlanId(config);

  auto port = createDefaultPortConfig(platformMapping, id, profileID, n);
  port.routable() = true;
  config.ports()->push_back(port);

  cfg::Vlan vlan;
  vlan.id() = n;
  vlan.name() = "vlan" + std::to_string(n);
  vlan.routable() = true;
  vlan.recordStats() = true;
  config.vlans()->push_back(vlan);

  cfg::VlanPort vlanPort;
  vlanPort.vlanID() = n;
  vlanPort.logicalPort() = static_cast<int32_t>(id);
  vlanPort.spanningTreeState() = cfg::SpanningTreeState::FORWARDING;
  vlanPort.emitTags() = false;
  config.vlanPorts()->push_back(vlanPort);

  cfg::Interface intf;
  intf.intfID() = n;
  intf.vlanID() = n;
  intf.type() = cfg::InterfaceType::VLAN;
  intf.routerID() = 0;
  intf.scope() = cfg::Scope::LOCAL;
  config.interfaces()->push_back(intf);

  return n;
}

void removePortsFromConfig(
    cfg::SwitchConfig& config,
    const std::set<PortID>& portsToRemove,
    PortRemovalMode mode,
    bool pruneEmptyVlansAndInterfaces) {
  // Phases:
  //  1. Disable mode: just mark the listed ports DISABLED and return.
  //  2. Erase mode: drop the listed ports and their vlanPorts, remembering
  //     which vlans lost a member.
  //  3. If pruneEmptyVlansAndInterfaces: drop each of those vlans that no
  //     surviving vlanPort still uses (except the default vlan), plus the VLAN
  //     interface on each.
  if (portsToRemove.empty()) {
    return;
  }

  // Disable mode: leave the port entries (and their vlanPorts/vlans/interfaces)
  // in place, just mark those ports DISABLED.
  if (mode == PortRemovalMode::Disable) {
    for (auto& p : *config.ports()) {
      if (portsToRemove.count(PortID(*p.logicalID())) > 0) {
        p.state() = cfg::PortState::DISABLED;
      }
    }
    return;
  }

  // Erase mode. Drop the ports.
  auto& ports = *config.ports();
  ports.erase(
      std::remove_if(
          ports.begin(),
          ports.end(),
          [&](const cfg::Port& p) {
            return portsToRemove.count(PortID(*p.logicalID())) > 0;
          }),
      ports.end());

  // Drop the removed ports' vlanPorts, remembering the vlans they were members
  // of -- those are the only vlans that could have become empty.
  std::set<int32_t> maybeEmptyVlans;
  auto& vlanPorts = *config.vlanPorts();
  vlanPorts.erase(
      std::remove_if(
          vlanPorts.begin(),
          vlanPorts.end(),
          [&](const cfg::VlanPort& vp) {
            if (portsToRemove.count(PortID(*vp.logicalPort())) > 0) {
              maybeEmptyVlans.insert(*vp.vlanID());
              return true;
            }
            return false;
          }),
      vlanPorts.end());

  if (!pruneEmptyVlansAndInterfaces) {
    return;
  }

  // A candidate vlan is dropped only if no surviving vlanPort still references
  // it and it is not the default vlan. This leaves originally-empty vlans
  // (never candidates) and vlans a surviving tagged/multi-vlan member still
  // uses, so no vlanPort is ever left pointing at a dropped vlan.
  std::set<int32_t> stillUsedVlans;
  for (const auto& vp : vlanPorts) {
    stillUsedVlans.insert(*vp.vlanID());
  }
  std::set<int32_t> vlansToRemove;
  for (const auto vlanId : maybeEmptyVlans) {
    if (vlanId != *config.defaultVlan() && stillUsedVlans.count(vlanId) == 0) {
      vlansToRemove.insert(vlanId);
    }
  }

  auto& vlans = *config.vlans();
  vlans.erase(
      std::remove_if(
          vlans.begin(),
          vlans.end(),
          [&](const cfg::Vlan& v) { return vlansToRemove.count(*v.id()) > 0; }),
      vlans.end());

  // Drop the VLAN interface sitting on each removed vlan. A port-router
  // (PORT-type) interface is bound to a port via portID (its vlanID field is
  // unset, defaulting to 0), so leave those untouched for now -- this helper
  // only manages VLAN-style configs. TODO: prune port-router interfaces once
  // platform support for them is verified.
  auto& interfaces = *config.interfaces();
  interfaces.erase(
      std::remove_if(
          interfaces.begin(),
          interfaces.end(),
          [&](const cfg::Interface& i) {
            if (i.portID().has_value()) {
              return false; // port-router interface: leave in place for now
            }
            return vlansToRemove.count(*i.vlanID()) > 0;
          }),
      interfaces.end());
}

} // namespace facebook::fboss::utility
