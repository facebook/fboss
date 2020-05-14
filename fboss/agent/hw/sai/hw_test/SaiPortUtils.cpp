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

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss::utility {

namespace {
SaiPortTraits::AdapterKey getPortAdapterKey(const HwSwitch* hw, PortID port) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto handle = saiSwitch->managerTable()->portManager().getPortHandle(port);
  CHECK(handle);
  return handle->port->adapterKey();
}

std::pair<std::string, cfg::PortProfileID> getMappingNameAndProfileID(
    const Platform* platform,
    PortID port,
    cfg::PortSpeed speed) {
  auto platformPort = platform->getPlatformPort(port);
  if (auto entry = platformPort->getPlatformPortEntry()) {
    return {*entry->mapping_ref()->name_ref(),
            platformPort->getProfileIDBySpeed(speed)};

  } else {
    throw FbossError("Port:", port, " doesn't have PlatformPortEntry");
  }
}

} // namespace
bool portEnabled(const HwSwitch* hw, PortID port) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::AdminState state;
  SaiApiTable::getInstance()->portApi().getAttribute(key, state);
  return state.value();
}

cfg::PortSpeed curPortSpeed(const HwSwitch* hw, PortID port) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::Speed speed;
  SaiApiTable::getInstance()->portApi().getAttribute(key, speed);
  return cfg::PortSpeed(speed.value());
}

void assertPort(
    const HwSwitch* hw,
    PortID port,
    bool enabled,
    cfg::PortSpeed speed) {
  CHECK_EQ(enabled, portEnabled(hw, port));
  if (enabled) {
    // Only verify speed on enabled ports
    CHECK_EQ(
        static_cast<int>(speed),
        static_cast<int>(utility::curPortSpeed(hw, port)));
  }
}

void assertPortStatus(const HwSwitch* hw, PortID port) {
  CHECK(portEnabled(hw, port));
}

void assertPortsLoopbackMode(
    const HwSwitch* hw,
    const std::map<PortID, int>& port2LoopbackMode) {
  for (auto portAndLoopBackMode : port2LoopbackMode) {
    assertPortLoopbackMode(
        hw, portAndLoopBackMode.first, portAndLoopBackMode.second);
  }
}

void assertPortSampleDestination(
    const HwSwitch* /*hw*/,
    PortID /*port*/,
    int /*expectedSampleDestination*/) {
  throw FbossError("sampling is unsupported for SAI");
}

void assertPortsSampleDestination(
    const HwSwitch* /*hw*/,
    const std::map<PortID, int>& /*port2SampleDestination*/) {
  throw FbossError("sampling is unsupported for SAI");
}

void assertPortLoopbackMode(
    const HwSwitch* hw,
    PortID port,
    int expectedLoopbackMode) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::InternalLoopbackMode loopbackMode;
  SaiApiTable::getInstance()->portApi().getAttribute(key, loopbackMode);
  CHECK_EQ(expectedLoopbackMode, loopbackMode.value());
}

void enableOneLane(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed /*disabledLaneSpeed*/,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  // remove all except first
  for (auto itr = allPortsinGroup.begin() + 1; itr != allPortsinGroup.end();
       itr++) {
    auto groupMemberPort = std::find_if(
        config->ports_ref()->begin(),
        config->ports_ref()->end(),
        [itr](auto port) {
          return static_cast<PortID>(*port.logicalID_ref()) == *itr;
        });
    config->ports_ref()->erase(groupMemberPort);
    auto vlanMemberPort = std::find_if(
        config->vlanPorts_ref()->begin(),
        config->vlanPorts_ref()->end(),
        [itr](auto vlanPort) {
          return static_cast<PortID>(*vlanPort.logicalPort_ref()) == *itr;
        });
    config->vlanPorts_ref()->erase(vlanMemberPort);
  }

  auto firstLanePort = std::find_if(
      config->ports_ref()->begin(),
      config->ports_ref()->end(),
      [&allPortsinGroup](auto port) {
        return static_cast<PortID>(*port.logicalID_ref()) == allPortsinGroup[0];
      });
  *firstLanePort->speed_ref() = enabledLaneSpeed;
  *firstLanePort->state_ref() = cfg::PortState::ENABLED;

  auto [name, profileID] = getMappingNameAndProfileID(
      platform,
      static_cast<PortID>(*firstLanePort->logicalID_ref()),
      enabledLaneSpeed);
  firstLanePort->name_ref() = name;
  *firstLanePort->profileID_ref() = profileID;
}

void enableAllLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  // keep all except
  auto portItr = allPortsinGroup.begin();
  int portIndex = 0;
  for (; portItr != allPortsinGroup.end(); portItr++, portIndex++) {
    *config->ports[portIndex].speed_ref() = enabledLaneSpeed;
    *config->ports[portIndex].state_ref() = cfg::PortState::ENABLED;

    auto [name, profileID] = getMappingNameAndProfileID(
        platform,
        static_cast<PortID>(*config->ports[portIndex].logicalID_ref()),
        enabledLaneSpeed);
    config->ports_ref()[portIndex].name_ref() = name;
    *config->ports[portIndex].profileID_ref() = profileID;
  }
}

void enableTwoLanes(
    cfg::SwitchConfig* config,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed /*disabledLaneSpeed*/,
    std::vector<PortID> allPortsinGroup,
    const Platform* platform) {
  // keep only first and third
  auto front = allPortsinGroup.front();
  auto portItr = config->ports_ref()->begin();
  while (portItr != config->ports_ref()->end()) {
    if ((static_cast<PortID>(*portItr->logicalID_ref()) % 2) != 0) {
      portItr = config->ports_ref()->erase(portItr);
    } else {
      *portItr->speed_ref() = enabledLaneSpeed;
      *portItr->state_ref() = cfg::PortState::ENABLED;

      auto [name, profileID] = getMappingNameAndProfileID(
          platform,
          static_cast<PortID>(*portItr->logicalID_ref()),
          enabledLaneSpeed);
      portItr->name_ref() = name;
      *portItr->profileID_ref() = profileID;
      portItr++;
    }
  }

  auto vlanPortItr = config->vlanPorts_ref()->begin();
  while (vlanPortItr != config->vlanPorts_ref()->end()) {
    if ((static_cast<PortID>(*vlanPortItr->logicalPort_ref()) - front) % 2 !=
        0) {
      vlanPortItr = config->vlanPorts_ref()->erase(vlanPortItr);
    } else {
      vlanPortItr++;
    }
  }
}

void assertQUADMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    std::vector<PortID> allPortsinGroup) {
  utility::assertPort(hw, allPortsinGroup[0], true, enabledLaneSpeed);
}

void assertDUALMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed /*disabledLaneSpeed*/,
    std::vector<PortID> allPortsinGroup) {
  bool odd_lane;
  auto portItr = allPortsinGroup.begin();
  for (; portItr != allPortsinGroup.end(); portItr++) {
    odd_lane = (*portItr - allPortsinGroup.front()) % 2 == 0 ? true : false;
    if (odd_lane) {
      utility::assertPort(hw, *portItr, true, enabledLaneSpeed);
    }
  }
}
void assertSINGLEMode(
    HwSwitch* hw,
    cfg::PortSpeed enabledLaneSpeed,
    cfg::PortSpeed /*disabledLaneSpeed*/,
    std::vector<PortID> allPortsinGroup) {
  utility::assertPort(hw, allPortsinGroup[0], true, enabledLaneSpeed);
}

} // namespace facebook::fboss::utility
