/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwPortUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"

namespace {
using namespace facebook::fboss;

const std::array<bool, 4> kAllPortsEnabled = {true, true, true, true};
const std::array<bool, 4> kFirstPortEnabled = {true, false, false, false};
const std::array<bool, 4> kOddPortsEnabled = {true, false, true, false};

std::pair<std::string, cfg::PortProfileID> getMappingNameAndProfileID(
    const Platform* platform,
    PortID port,
    cfg::PortSpeed speed) {
  auto platformPort = platform->getPlatformPort(port);
  const auto& entry = platformPort->getPlatformPortEntry();
  return {*entry.mapping()->name(), platformPort->getProfileIDBySpeed(speed)};
}
} // namespace

namespace facebook::fboss::utility {

void updateFlexConfig(
    cfg::SwitchConfig* config,
    FlexPortMode flexMode,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  if (flexMode == FlexPortMode::FOURX10G) {
    enablePortsInPortGroup(
        config,
        cfg::PortSpeed::XG,
        cfg::PortSpeed::XG,
        allPortsinGroup,
        platform,
        kAllPortsEnabled);
  } else if (flexMode == FlexPortMode::FOURX25G) {
    enablePortsInPortGroup(
        config,
        cfg::PortSpeed::TWENTYFIVEG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup,
        platform,
        kAllPortsEnabled);
  } else if (flexMode == FlexPortMode::ONEX40G) {
    enablePortsInPortGroup(
        config,
        cfg::PortSpeed::FORTYG,
        cfg::PortSpeed::XG,
        allPortsinGroup,
        platform,
        kFirstPortEnabled);
  } else if (flexMode == FlexPortMode::TWOX50G) {
    enablePortsInPortGroup(
        config,
        cfg::PortSpeed::FIFTYG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup,
        platform,
        kOddPortsEnabled);
  } else if (flexMode == FlexPortMode::ONEX100G) {
    enablePortsInPortGroup(
        config,
        cfg::PortSpeed::HUNDREDG,
        cfg::PortSpeed::TWENTYFIVEG,
        allPortsinGroup,
        platform,
        kFirstPortEnabled);
  } else if (flexMode == FlexPortMode::ONEX400G) {
    enablePortsInPortGroup(
        config,
        cfg::PortSpeed::FOURHUNDREDG,
        cfg::PortSpeed::HUNDREDG,
        allPortsinGroup,
        platform,
        kFirstPortEnabled);
  } else {
    throw FbossError("invalid FlexConfig Mode");
  }
}

bool portsExistsInPortGroup(
    const Platform* platform,
    const std::vector<PortID>& allPortsInGroup,
    cfg::PortSpeed speed) {
  for (auto portIdx = 0; portIdx < allPortsInGroup.size(); portIdx++) {
    auto portID = allPortsInGroup.at(portIdx);
    const auto& platformPortEntry =
        platform->getPlatformPort(portID)->getPlatformPortEntry();
    for (const auto& profile : *platformPortEntry.supportedProfiles()) {
      auto profileID = profile.first;
      if (auto profileCfg = platform->getPortProfileConfig(
              PlatformPortProfileConfigMatcher(profileID, portID))) {
        if (*profileCfg->speed() == speed) {
          return true;
        }
      }
    }
  }
  return false;
}

void enablePortsInPortGroup(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed disabledLaneSpeed,
    const std::vector<PortID>& allPortsInGroup,
    const Platform* platform,
    const std::array<bool, 4>& enabledPortsOption) {
  // Manipulate the port configs based on enabledPortsOption
  // If the port is enabled, then update the speed/state/profileID/name
  // If the port is disabled:
  // 1) If the platform supports adding or removing port, remove this port from
  //    the config
  // 2) Otherwise, update the speed with disabledLaneSpeed and state with
  //    cfg::PortState::DISABLED
  for (auto portIdx = 0; portIdx < allPortsInGroup.size(); portIdx++) {
    auto portID = allPortsInGroup.at(portIdx);
    bool isPortEnabled = enabledPortsOption.at(portIdx);
    auto portCfgIt = std::find_if(
        config->ports()->begin(), config->ports()->end(), [portID](auto port) {
          return static_cast<PortID>(*port.logicalID()) == portID;
        });
    if (portCfgIt == config->ports()->end()) {
      if (isPortEnabled || !platform->supportsAddRemovePort()) {
        throw FbossError("Port:", portID, " doesn't exist in the config");
      }
      // Skip if disabled port doesn't exist
      continue;
    }

    // If the port is disabled and the platform support removing ports
    if (!isPortEnabled && platform->supportsAddRemovePort()) {
      config->ports()->erase(portCfgIt);
      auto vlanMemberPort = std::find_if(
          config->vlanPorts()->begin(),
          config->vlanPorts()->end(),
          [portID](auto vlanPort) {
            return static_cast<PortID>(*vlanPort.logicalPort()) == portID;
          });
      if (vlanMemberPort != config->vlanPorts()->end()) {
        config->vlanPorts()->erase(vlanMemberPort);
      }
      continue;
    }

    // For the remaining ports which doesn't get erased, update port config
    portCfgIt->state() =
        isPortEnabled ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
    portCfgIt->speed() = isPortEnabled ? enabledLaneSpeed : disabledLaneSpeed;

    auto [name, profileID] =
        getMappingNameAndProfileID(platform, portID, *portCfgIt->speed());
    portCfgIt->name() = name;
    portCfgIt->profileID() = profileID;
  }
}
} // namespace facebook::fboss::utility
