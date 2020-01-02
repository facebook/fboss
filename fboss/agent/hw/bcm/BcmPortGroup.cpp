/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPortGroup.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace {
using facebook::fboss::BcmPortGroup;
using facebook::fboss::FbossError;
using facebook::fboss::LaneSpeeds;
using facebook::fboss::cfg::PortSpeed;
BcmPortGroup::LaneMode neededLaneModeForSpeed(
    PortSpeed speed,
    LaneSpeeds laneSpeeds) {
  if (speed == PortSpeed::DEFAULT) {
    throw FbossError("Speed cannot be DEFAULT if flexports are enabled");
  }

  for (auto& laneSpeed : laneSpeeds) {
    auto dv = std::div(static_cast<int>(speed), static_cast<int>(laneSpeed));
    if (dv.rem) {
      // skip if this requires an unsupported lane speed
      continue;
    }

    auto neededLanes = dv.quot;
    if (neededLanes == 1) {
      return BcmPortGroup::LaneMode::SINGLE;
    } else if (neededLanes == 2) {
      return BcmPortGroup::LaneMode::DUAL;
    } else if (neededLanes > 2 && neededLanes <= 4) {
      return BcmPortGroup::LaneMode::QUAD;
    }
  }

  throw FbossError("Cannot support speed ", speed);
}

void checkLaneModeisValid(int lane, BcmPortGroup::LaneMode desiredMode) {
  if (desiredMode == BcmPortGroup::LaneMode::QUAD) {
    if (lane != 0) {
      throw FbossError("Only lane 0 can be enabled in QUAD mode");
    }
  } else if (desiredMode == BcmPortGroup::LaneMode::DUAL) {
    if (lane != 0 && lane != 2) {
      throw FbossError("Only lanes 0 or 2 can be enabled in DUAL mode");
    }
  }
}

} // namespace

namespace facebook::fboss {

BcmPortGroup::BcmPortGroup(
    BcmSwitch* hw,
    BcmPort* controllingPort,
    std::vector<BcmPort*> allPorts)
    : hw_(hw),
      controllingPort_(controllingPort),
      allPorts_(std::move(allPorts)) {
  // We expect all ports to run at the same speed and are passed in in
  // the correct order.
  portSpeed_ = controllingPort_->getSpeed();
  // Instead of demanding the input ports list to be ordered by lane. We can
  // just sort the list based on the port id since we always assign port id
  // in the order of the physical lane order
  std::sort(
      allPorts_.begin(),
      allPorts_.end(),
      [](const auto& lPort, const auto& rPort) {
        return lPort->getPortID() < rPort->getPortID();
      });
  // If any port is enabled in the list, the speed of all the ports should be
  // the same as controling port
  for (const auto& port : allPorts_) {
    if (port->isEnabled() && portSpeed_ != port->getSpeed()) {
      throw FbossError("All enabled ports must have same speed");
    }
  }

  // get the number of active lanes
  auto activeLanes = retrieveActiveLanes();
  laneMode_ = numLanesToLaneMode(activeLanes);

  XLOG(INFO) << "Create BcmPortGroup with controlling port: "
             << controllingPort->getPortID()
             << ", port group size: " << allPorts_.size();
}

BcmPortGroup::~BcmPortGroup() {}

BcmPortGroup::LaneMode BcmPortGroup::numLanesToLaneMode(uint8_t numLanes) {
  try {
    return static_cast<LaneMode>(numLanes);
  } catch (const std::exception& ex) {
    throw FbossError(
        "Unexpected number of lanes retrieved for bcm port ", numLanes);
  }
}

BcmPortGroup::LaneMode BcmPortGroup::calculateDesiredLaneMode(
    const std::vector<std::shared_ptr<Port>>& ports,
    LaneSpeeds laneSpeeds) {
  auto desiredMode = LaneMode::SINGLE;
  for (int lane = 0; lane < ports.size(); ++lane) {
    auto port = ports[lane];
    if (port->isEnabled()) {
      auto neededMode = neededLaneModeForSpeed(port->getSpeed(), laneSpeeds);
      if (neededMode > desiredMode) {
        desiredMode = neededMode;
      }

      checkLaneModeisValid(lane, desiredMode);
      XLOG(DBG3) << "Port " << port->getID() << " enabled with speed "
                 << static_cast<int>(port->getSpeed());
    }
  }
  return desiredMode;
}

BcmPortGroup::LaneMode BcmPortGroup::calculateDesiredLaneModeFromConfig(
    const std::vector<std::shared_ptr<Port>>& ports,
    const std::map<cfg::PortProfileID, phy::PortProfileConfig>&
        supportedProfiles) {
  // As we support more and more platforms, the existing lane mode calculation
  // won't be valid any more. For example, for 100G port, we can use 2x50PAM4 or
  // 4x25NRZ mode. Therefore, we introduced the new PlatformPort design, which
  // port will have a new field called `profileID`. With this new speed
  // profile, we can understand how many lanes for such speed on this port from
  // the config.
  auto desiredMode = LaneMode::SINGLE;
  for (auto port : ports) {
    if (port->isEnabled()) {
      auto profileCfg = supportedProfiles.find(port->getProfileID());
      if (profileCfg == supportedProfiles.end()) {
        throw FbossError(
            "Port: ",
            port->getName(),
            ", has unsupported speed profile: ",
            apache::thrift::util::enumNameSafe(port->getProfileID()));
      }
      auto neededMode = numLanesToLaneMode(profileCfg->second.iphy.numLanes);
      if (neededMode > desiredMode) {
        desiredMode = neededMode;
      }
    }
  }
  return desiredMode;
}

std::vector<std::shared_ptr<Port>> BcmPortGroup::getSwPorts(
    const std::shared_ptr<SwitchState>& state) const {
  std::vector<std::shared_ptr<Port>> ports;
  // with the new data platform config design, we can get all the ports from the
  // same port group from the config.
  if (auto platformPorts =
          hw_->getPlatform()->config()->thrift.platform.platformPorts_ref()) {
    const auto& portList = utility::getPlatformPortsByControllingPort(
        *platformPorts, controllingPort_->getPortID());
    for (const auto& port : portList) {
      auto swPort = state->getPorts()->getPortIf(PortID(port.mapping.id));
      // Platform port doesn't exist in sw config, no need to program
      if (swPort) {
        ports.push_back(swPort);
      }
    }
  } else {
    for (auto bcmPort : allPorts_) {
      auto swPort = bcmPort->getSwitchStatePort(state);
      // Make sure the ports support the configured speed.
      // We check this even if the port is disabled.
      if (!bcmPort->supportsSpeed(swPort->getSpeed())) {
        throw FbossError(
            "Port ",
            swPort->getID(),
            " does not support speed ",
            static_cast<int>(swPort->getSpeed()));
      }
      ports.push_back(swPort);
    }
  }
  return ports;
}

uint8_t BcmPortGroup::getLane(const BcmPort* bcmPort) const {
  return bcmPort->getBcmPortId() - controllingPort_->getBcmPortId();
}

bool BcmPortGroup::validConfiguration(
    const std::shared_ptr<SwitchState>& state) const {
  try {
    const auto& ports = getSwPorts(state);
    // TODO(joseph5wu) Once we roll out new config everywhere, we can get rid of
    // the old way to calculate lane mode
    if (auto supportedProfiles =
            hw_->getPlatform()
                ->config()
                ->thrift.platform.supportedProfiles_ref()) {
      calculateDesiredLaneModeFromConfig(ports, *supportedProfiles);
    } else {
      calculateDesiredLaneMode(ports, controllingPort_->supportedLaneSpeeds());
    }
  } catch (const std::exception& ex) {
    XLOG(DBG1) << "Received exception determining lane mode: " << ex.what();
    return false;
  }
  return true;
}

void BcmPortGroup::reconfigureIfNeeded(
    const std::shared_ptr<SwitchState>& state) {
  // This logic is a bit messy. We could encode some notion of port
  // groups into the swith state somehow so it is easy to generate
  // deltas for these. For now, we need pass around the SwitchState
  // object and get the relevant ports manually.
  auto ports = getSwPorts(state);
  // ports is guaranteed to be the same size as allPorts_
  auto speedChanged = ports[0]->getSpeed() != portSpeed_;

  LaneMode desiredLaneMode;
  // TODO(joseph5wu) Once we roll out new config everywhere, we can get rid of
  // the old way to calculate lane mode
  if (auto supportedProfiles = hw_->getPlatform()
                                   ->config()
                                   ->thrift.platform.supportedProfiles_ref()) {
    desiredLaneMode =
        calculateDesiredLaneModeFromConfig(ports, *supportedProfiles);
  } else {
    desiredLaneMode = calculateDesiredLaneMode(
        ports, controllingPort_->supportedLaneSpeeds());
  }

  if (speedChanged) {
    controllingPort_->getPlatformPort()->linkSpeedChanged(ports[0]->getSpeed());
  }
  if (desiredLaneMode != laneMode_) {
    reconfigureLaneMode(ports, desiredLaneMode);
  }
}

void BcmPortGroup::reconfigureLaneMode(
    const std::vector<std::shared_ptr<Port>>& ports,
    LaneMode newLaneMode) {
  // The logic for this follows the steps required for flex-port support
  // outlined in the sdk documentation.
  XLOG(DBG1) << "Reconfiguring port " << controllingPort_->getBcmPortId()
             << " from using " << laneMode_ << " lanes to " << newLaneMode
             << " lanes";

  // TODO(joseph5wu): Right now, we always assume we can get the swPort from
  // the sw state, which means we don't support the case that we can remove
  // a port from sw state yet.
  auto getSwPort = [&ports](PortID id) -> std::shared_ptr<Port> {
    for (auto port : ports) {
      if (port->getID() == id) {
        return port;
      }
    }
    throw FbossError("Can't find sw port: ", id);
  };

  // 1. Disable linkscan, then disable ports.
  for (auto& bcmPort : allPorts_) {
    auto swPort = getSwPort(bcmPort->getPortID());
    bcmPort->disableLinkscan();
    bcmPort->disable(swPort);
  }

  // 2. Set the opennslPortControlLanes setting
  setActiveLanes(ports, newLaneMode);

  // 3. Only enable linkscan, and don't enable ports.
  // Enable port will program the port with the sw config and also adding it
  // to vlan, which means there's a dependency on vlan readiness. Therefore,
  // we should let the caller to decide when it's the best time to enable port,
  // usually the very end of BcmSwitch::stateChangedImpl()
  for (auto& bcmPort : allPorts_) {
    auto swPort = getSwPort(bcmPort->getPortID());
    if (swPort->isEnabled()) {
      bcmPort->enableLinkscan();
    }
  }
}

} // namespace facebook::fboss
