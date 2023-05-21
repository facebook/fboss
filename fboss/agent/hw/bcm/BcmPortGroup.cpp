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
#include "fboss/agent/hw/bcm/BcmPortResourceBuilder.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace {
using facebook::fboss::BcmPlatform;
using facebook::fboss::BcmPortGroup;
using facebook::fboss::FbossError;
using facebook::fboss::LaneSpeeds;
using facebook::fboss::Port;
using facebook::fboss::PortID;
using facebook::fboss::cfg::PortSpeed;
using facebook::fboss::phy::VCOFrequency;
using facebook::fboss::phy::VCOFrequencyFactor;

void setLanesControl(int unit, bcm_port_t port, unsigned lanes) {
  int rv = bcm_port_control_set(unit, port, bcmPortControlLanes, lanes);
  facebook::fboss::bcmCheckError(
      rv, "Failed to configure ", lanes, " active lanes for bcm port", port);
}

std::shared_ptr<Port> getSwPortIf(
    const std::vector<std::shared_ptr<Port>>& ports,
    PortID id) {
  for (auto port : ports) {
    if (port->getID() == id) {
      return port;
    }
  }
  return nullptr;
}

std::shared_ptr<Port> getSwPort(
    const std::vector<std::shared_ptr<Port>>& ports,
    PortID id) {
  auto port = getSwPortIf(ports, id);
  if (port) {
    return port;
  }

  throw FbossError("Can't find sw port: ", id);
}

VCOFrequency getVCOFrequency(
    const BcmPlatform* platform,
    facebook::fboss::cfg::PortProfileID profile,
    PortID portID) {
  VCOFrequencyFactor factor;

  auto config = platform->getPortProfileConfig(
      facebook::fboss::PlatformPortProfileConfigMatcher(profile, portID));
  if (!config) {
    throw FbossError(
        "Unsupported speed profile: ",
        apache::thrift::util::enumNameSafe(profile));
  }
  factor.speed() = *config->speed();
  factor.fecMode() = *config->iphy()->fec();
  return platform->getVCOFrequency(factor);
}

std::string VCOFrequencySetToString(std::set<VCOFrequency> vcos) {
  std::stringstream stream;
  bool first = true;
  stream << "{";
  for (auto vco : vcos) {
    if (!first) {
      stream << ", ";
    }
    first = false;
    stream << apache::thrift::util::enumNameSafe(vco);
  }
  stream << "}";
  return stream.str();
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
  // Instead of demanding the input ports list to be ordered by lane. We can
  // just sort the list based on the port id since we always assign port id
  // in the order of the physical lane order
  std::sort(
      allPorts_.begin(),
      allPorts_.end(),
      [](const auto& lPort, const auto& rPort) {
        return lPort->getPortID() < rPort->getPortID();
      });

  // get the number of active lanes
  auto activeLanes = retrieveActiveLanes();
  laneMode_ = numLanesToLaneMode(activeLanes);

  // get the current HW VCO frequencies
  if (hw_->getPlatform()->supportsAddRemovePort()) {
    for (const auto* bcmPort : allPorts_) {
      vcoFrequencies_.insert(getVCOFrequency(
          hw_->getPlatform(),
          bcmPort->getCurrentProfile(),
          bcmPort->getPortID()));
    }
  }

  XLOG(DBG2) << "Create BcmPortGroup with controlling port: "
             << controllingPort->getPortID()
             << ", port group size: " << allPorts_.size()
             << ", laneMode:" << laneMode_
             << (vcoFrequencies_.empty()
                     ? "."
                     : folly::sformat(
                           ", VCO Frequencies {}.",
                           VCOFrequencySetToString(vcoFrequencies_)));
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
    const PlatformMapping* platformMapping) {
  auto platformPorts = platformMapping->getPlatformPorts();
  // As we support more and more platforms, the existing lane mode calculation
  // won't be valid any more. For example, for 100G port, we can use 2x50PAM4 or
  // 4x25NRZ mode. Therefore, we introduced the new PlatformPort design, which
  // port will have a new field called `profileID`. With this new speed
  // profile, we can understand how many lanes for such speed on this port from
  // the config.
  auto desiredMode = LaneMode::SINGLE;
  // make sure subsumed ports are disabled
  std::set<int32_t> needSubsumedPorts;
  for (auto port : ports) {
    // TODO(joseph5wu) Still waiting for landing the change to support bcm_test
    // generate the full config with the proper profileID.
    if (port->getAdminState() == cfg::PortState::DISABLED &&
        port->getProfileID() == cfg::PortProfileID::PROFILE_DEFAULT) {
      continue;
    }

    // First check whether the Platform supports such profileID
    auto profileCfg = platformMapping->getPortProfileConfig(
        PlatformPortProfileConfigMatcher(port->getProfileID(), port->getID()));
    if (!profileCfg.has_value()) {
      throw FbossError(
          "Port: ",
          port->getName(),
          ", uses platform-unsupported speed profile: ",
          apache::thrift::util::enumNameSafe(port->getProfileID()));
    }
    // Then check whether the PlatformPort supports such profileID
    auto itPlatformPort = platformPorts.find(port->getID());
    if (itPlatformPort == platformPorts.end()) {
      throw FbossError(
          "Port: ",
          port->getName(),
          ", doesn't have corresponding PlatformPortEntry");
    }
    auto itPortProfileCfg =
        itPlatformPort->second.supportedProfiles()->find(port->getProfileID());
    if (itPortProfileCfg == itPlatformPort->second.supportedProfiles()->end()) {
      throw FbossError(
          "Port: ",
          port->getName(),
          ", uses port-unsupported speed profile: ",
          apache::thrift::util::enumNameSafe(port->getProfileID()));
    }
    if (auto subsumedPorts = itPortProfileCfg->second.subsumedPorts()) {
      for (auto subsumedPort : *subsumedPorts) {
        needSubsumedPorts.insert(subsumedPort);
      }
    }

    auto neededMode = numLanesToLaneMode(*profileCfg->iphy()->numLanes());
    if (neededMode > desiredMode) {
      desiredMode = neededMode;
    }
  }

  // Check whether all the needSubsumedPorts are disabled
  for (auto port : ports) {
    if (needSubsumedPorts.find(port->getID()) != needSubsumedPorts.end() &&
        port->getAdminState() != cfg::PortState::DISABLED) {
      throw FbossError(
          "Port: ",
          port->getName(),
          " needs to be subsumed but the admin state is not disabled");
    }
  }
  return desiredMode;
}

std::vector<std::shared_ptr<Port>> BcmPortGroup::getSwPorts(
    const std::shared_ptr<SwitchState>& state) const {
  std::vector<std::shared_ptr<Port>> ports;
  // with the new data platform config design, we can get all the ports from the
  // same port group from the config.
  const auto& platformPorts = hw_->getPlatform()->getPlatformPorts();
  if (!platformPorts.empty()) {
    const auto& portList = utility::getPlatformPortsByControllingPort(
        platformPorts, controllingPort_->getPortID());
    for (const auto& port : portList) {
      auto swPort = state->getMultiSwitchPorts()->getNodeIf(
          PortID(*port.mapping()->id()));
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
    calculateDesiredLaneMode(ports, hw_->getPlatform()->getPlatformMapping());
  } catch (const std::exception& ex) {
    XLOG(DBG1) << "Received exception determining lane mode: " << ex.what();
    return false;
  }
  return true;
}

void BcmPortGroup::reconfigureIfNeeded(
    const std::shared_ptr<SwitchState>& oldState,
    const std::shared_ptr<SwitchState>& newState) {
  // This logic is a bit messy. We could encode some notion of port
  // groups into the swith state somehow so it is easy to generate
  // deltas for these. For now, we need pass around the SwitchState
  // object and get the relevant ports manually.
  auto oldPorts = getSwPorts(oldState);
  auto newPorts = getSwPorts(newState);

  LaneMode desiredLaneMode = calculateDesiredLaneMode(
      newPorts, hw_->getPlatform()->getPlatformMapping());

  bool needsVCOChange = false;
  std::set<phy::VCOFrequency> desiredVCOFrequencies;
  if (hw_->getPlatform()->supportsAddRemovePort()) {
    for (const auto& port : newPorts) {
      desiredVCOFrequencies.insert(getVCOFrequency(
          hw_->getPlatform(), port->getProfileID(), port->getID()));
    }
    needsVCOChange = (vcoFrequencies_ != desiredVCOFrequencies);
  }

  if (desiredLaneMode != laneMode_ || needsVCOChange ||
      oldPorts.size() != newPorts.size()) {
    // reprogram port group if we have different lane configuration or we need
    // a VCO change. Otherwise bcm port can handle it
    reconfigure(oldPorts, newPorts, desiredLaneMode, desiredVCOFrequencies);
  }
}

void BcmPortGroup::reconfigure(
    const std::vector<std::shared_ptr<Port>>& oldPorts,
    const std::vector<std::shared_ptr<Port>>& newPorts,
    LaneMode newLaneMode,
    std::set<phy::VCOFrequency>& newVCOFrequencies) {
  // The logic for this follows the steps required for flex-port support
  // outlined in the sdk documentation.
  XLOG(DBG1) << "Reconfiguring port " << controllingPort_->getBcmPortId()
             << " from using " << laneMode_ << " lanes to " << newLaneMode
             << " lanes"
             << (newVCOFrequencies.empty()
                     ? "."
                     : folly::sformat(
                           ", from {} to {}.",
                           VCOFrequencySetToString(vcoFrequencies_),
                           VCOFrequencySetToString(newVCOFrequencies)));

  // 1. For all existing ports, disable linkscan, then disable
  for (auto& bcmPort : allPorts_) {
    auto swPort = getSwPort(oldPorts, bcmPort->getPortID());
    // CS00011459055 Broadcom recommends to disable loopback before calling
    // flex port api
    bcmPort->setLoopbackMode(cfg::PortLoopbackMode::NONE);
    bcmPort->disableLinkscan();
    bcmPort->disable(swPort);
  }

  // 2. Set the bcmPortControlLanes setting
  setActiveLanes(newPorts, newLaneMode);

  // 3. Only enable linkscan, and don't enable ports.
  // Enable port will program the port with the sw config and also adding it
  // to vlan, which means there's a dependency on vlan readiness. Therefore,
  // we should let the caller to decide when it's the best time to enable port,
  // usually the very end of BcmSwitch::stateChangedImpl()
  // (only do this for ports that exist in the new state)
  for (auto& swPort : newPorts) {
    auto bcmPort = hw_->getPortTable()->getBcmPort(swPort->getID());
    if (swPort->isEnabled()) {
      bcmPort->enableLinkscan();
    }
  }

  // 4. Update the vco frequencies set if needed
  if (!newVCOFrequencies.empty()) {
    XLOG(DBG2) << "Set PortGroup VCOFrequency, controlling port:"
               << controllingPort_->getBcmPortId() << ", from "
               << VCOFrequencySetToString(vcoFrequencies_) << " to "
               << VCOFrequencySetToString(newVCOFrequencies);
    vcoFrequencies_ = std::move(newVCOFrequencies);
  }
}

int BcmPortGroup::retrieveActiveLanes() const {
  if (controllingPort_->getPlatformPort()->shouldUsePortResourceAPIs()) {
    bcm_port_resource_t portResource;
    bcm_port_resource_t_init(&portResource);
    int rv = bcm_port_resource_get(
        hw_->getUnit(), controllingPort_->getBcmGport(), &portResource);
    bcmCheckError(
        rv,
        "Failed to get the number of active lanes using port resource for port ",
        controllingPort_->getBcmGport());
    return portResource.lanes;
  } else {
    int activeLanes;
    int rv = bcm_port_control_get(
        hw_->getUnit(),
        controllingPort_->getBcmPortId(),
        bcmPortControlLanes,
        &activeLanes);
    bcmCheckError(
        rv,
        "Failed to get the number of active lanes for port ",
        controllingPort_->getBcmPortId());
    return activeLanes;
  }
}

void BcmPortGroup::setActiveLanes(
    const std::vector<std::shared_ptr<Port>>& ports,
    LaneMode desiredLaneMode) {
  if (controllingPort_->getPlatformPort()->shouldUsePortResourceAPIs()) {
    if (!hw_->getPlatform()->supportsAddRemovePort()) {
      // To set a new active lanes, we might need to remove and add ports via
      // the brand new port resource api. If the platform port doesn't support
      // to add or remove port, we should throw error.
      throw FbossError(
          "Port: ",
          controllingPort_->getPortID(),
          " doesn't support add or remove ports via port resource apis");
    }
    setActiveLanesWithFlexPortApi(ports, desiredLaneMode);
  } else {
    // If the platform port doesn't support port resource apis, fall back to
    // use legacy way to change lane mode just for control port.
    /* The sdk has complex rules for which port configurations are valid
     * and how to transition between modes. Here are the supported
     * modes, copied from trident2.c:
     *
     *  Each TSC can be configured into following 5 mode:
     *   Lane number    0    1    2    3
     *   ------------  ---  ---  ---  ---
     *    single port  40G   x    x    x  (quad lane mode)
     *      dual port  20G   x   20G   x  (dual lane mode)
     *   tri_023 port  20G   x   10G  10G
     *   tri_012 port  10G  10G  20G   x
     *      quad port  10G  10G  10G  10G (single lane mode)
     *
     * The sdk also does not support going directly from a quad port to
     * a dual port, or vice versa. See trident2.c for more details.
     *
     * Note that we are not explicitly supporting tri_012 or tri_023
     * modes in fboss.
     */
    bcm_port_t basePort = controllingPort_->getBcmPortId();
    if ((laneMode_ == LaneMode::SINGLE && desiredLaneMode == LaneMode::DUAL) ||
        (laneMode_ == LaneMode::DUAL && desiredLaneMode == LaneMode::SINGLE)) {
      // We can't go directly from single to dual or vice versa, so just
      // configure it into quad mode first. This isn't the recommended
      // path in the sdk, but I tested it a bunch and it works the
      // same. The ports should all be disabled during this call anyways,
      // so we don't have to worry about packet loss during the transition.
      setLanesControl(hw_->getUnit(), basePort, 4);
    }
    setLanesControl(hw_->getUnit(), basePort, desiredLaneMode);
    laneMode_ = desiredLaneMode;
  }
}

void BcmPortGroup::setActiveLanesWithFlexPortApi(
    const std::vector<std::shared_ptr<Port>>& ports,
    LaneMode desiredLaneMode) {
  auto portResBuilder = std::make_unique<BcmPortResourceBuilder>(
      hw_, controllingPort_, desiredLaneMode);

  // First remove all the existing ports
  portResBuilder->removePorts(allPorts_);
  // And then add the new ports
  const auto& addedPorts = portResBuilder->addPorts(ports);
  // Finally program them all at once
  portResBuilder->program();

  auto controllingPortID = controllingPort_->getPortID();
  // Since we've done some port add/remove ops, we need to update port table
  // Remove all old entries from the port table, then add back the
  // newly added ports
  for (auto port : allPorts_) {
    hw_->writablePortTable()->removeBcmPort(port->getPortID());
  }
  std::vector<BcmPort*> newPorts;
  for (auto port : addedPorts) {
    // write it to port table
    hw_->writablePortTable()->addBcmPort(port->getID(), false /* warmboot */);
    // make sure it eixsts in port table
    auto* newPort = hw_->getPortTable()->getBcmPort(port->getID());
    newPorts.push_back(newPort);
  }

  // Then we need to update current port group to the new state
  controllingPort_ = hw_->getPortTable()->getBcmPort(controllingPortID);
  int beforePortGroupSize = allPorts_.size();
  allPorts_ = std::move(newPorts);
  // Finally register this portgroup to all the members
  for (auto& member : allPorts_) {
    member->registerInPortGroup(this);
  }

  // update laneMode_ to the new desiredMode
  XLOG(DBG2) << "Finished reconfiguring port group of control port: "
             << controllingPort_->getPortID() << ", from " << laneMode_
             << " lanes to " << desiredLaneMode << " lanes"
             << ", port group size from " << beforePortGroupSize << " to "
             << allPorts_.size();
  laneMode_ = desiredLaneMode;
}

} // namespace facebook::fboss
