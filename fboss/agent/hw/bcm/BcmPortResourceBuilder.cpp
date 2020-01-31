/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPortResourceBuilder.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {

BcmPortResourceBuilder::BcmPortResourceBuilder(
    BcmSwitch* hw,
    BcmPort* controllingPort,
    BcmPortGroup::LaneMode desiredLaneMode)
    : hw_(hw),
      controllingPort_(controllingPort),
      desiredLaneMode_(desiredLaneMode) {
  // Get the base physical_port later, which is the smallest among removed ports
}

void BcmPortResourceBuilder::removePorts(const std::vector<BcmPort*>& ports) {
  // BRCM SDK limits all the remove ports should be in front of the final array
  if (!ports_.empty()) {
    throw FbossError(
        "All the removed ports should be in front of the final array. ",
        "Current array size:",
        ports_.size());
  }

  const auto& chips = hw_->getPlatform()->getDataPlanePhyChips();
  if (chips.empty()) {
    throw FbossError("Not platform data plane phy chips");
  }

  // Get the base physical_port, which is the smallest among allPorts_
  // We need to put those need to delete first and set the physical_port to -1.
  for (auto port : ports) {
    bcm_port_resource_t oldPortRes;
    bcm_port_resource_t_init(&oldPortRes);
    auto rv = bcm_port_resource_speed_get(
        hw_->getUnit(), port->getBcmGport(), &oldPortRes);
    bcmCheckError(
        rv,
        "Can't get bcm_port_resource_speed for port:",
        port->getBcmPortId(),
        ", gport:",
        port->getBcmGport());
    // The base physical port should always be the smallest port number
    if (basePhysicalPort_ == -1 ||
        basePhysicalPort_ > oldPortRes.physical_port) {
      basePhysicalPort_ = oldPortRes.physical_port;
    }
    oldPortRes.physical_port = -1; /* -1: delete port */
    ports_.push_back(oldPortRes);
  }
  numRemovedPorts_ = ports.size();
}

std::vector<std::shared_ptr<Port>> BcmPortResourceBuilder::filterSubSumedPorts(
    const std::vector<std::shared_ptr<Port>>& ports) {
  // Get all subsumed ports
  std::set<PortID> subsumedPortSet;
  std::vector<std::shared_ptr<Port>> filteredPorts;
  for (const auto& port : ports) {
    auto platformPortEntry = hw_->getPlatform()
                                 ->getPlatformPort(port->getID())
                                 ->getPlatformPortEntry();
    if (!platformPortEntry) {
      throw FbossError(
          "Port: ",
          port->getID(),
          " doesn't have PlatformPortEntry. Not allowed to use flex port api");
    }

    const auto& itProfile =
        (*platformPortEntry).supportedProfiles.find(port->getProfileID());
    if (itProfile != (*platformPortEntry).supportedProfiles.end()) {
      // Gather all subsumed ports in the same group and remove them later
      if (auto subsumedPorts = itProfile->second.subsumedPorts_ref()) {
        for (auto portID : *subsumedPorts) {
          subsumedPortSet.insert(PortID(portID));
        }
      }
    } else if (port->isEnabled()) {
      // Make sure enabled port profile is allowed in the supported profile
      throw FbossError(
          "Enabled Port: ",
          (*platformPortEntry).mapping.name,
          " does not support speed profile: ",
          apache::thrift::util::enumNameSafe(port->getProfileID()));
    }
  }

  // Only add ports are not in the subsumed list into the final list
  for (auto port : ports) {
    if (subsumedPortSet.find(port->getID()) == subsumedPortSet.end()) {
      filteredPorts.push_back(port);
    }
  }

  return filteredPorts;
}

int BcmPortResourceBuilder::getBaseLane(PortID portID) {
  auto platformPortEntry =
      hw_->getPlatform()->getPlatformPort(portID)->getPlatformPortEntry();
  if (!platformPortEntry) {
    throw FbossError(
        "Port: ",
        portID,
        " doesn't have PlatformPortEntry. Not allowed to use flex port api");
  }
  const auto& iphyLanes = utility::getOrderedIphyLanes(
      *platformPortEntry, hw_->getPlatform()->getDataPlanePhyChips());
  return iphyLanes[0].lane;
}

std::vector<std::shared_ptr<Port>> BcmPortResourceBuilder::addPorts(
    const std::vector<std::shared_ptr<Port>>& ports) {
  // BRCM SDK limits all the remove ports should be in front of the final array
  if (ports_.empty()) {
    throw FbossError(
        "All the added ports should be after removed ports in the final array.",
        " Current array is empty");
  }
  auto filteredPorts = filterSubSumedPorts(ports);

  const auto& chips = hw_->getPlatform()->getDataPlanePhyChips();
  if (chips.empty()) {
    throw FbossError("Not platform data plane phy chips");
  }

  int basePhysicalLane_{-1};
  for (const auto& port : ports) {
    auto baseLane = getBaseLane(port->getID());
    if (basePhysicalLane_ == -1 || basePhysicalLane_ > baseLane) {
      basePhysicalLane_ = baseLane;
    }
  }

  for (auto port : filteredPorts) {
    auto profileConf =
        hw_->getPlatform()->getPortProfileConfig(port->getProfileID());
    if (!profileConf) {
      throw FbossError(
          "Port: ",
          port->getName(),
          " demands unsupported profile: ",
          apache::thrift::util::enumNameSafe(port->getProfileID()));
    }
    auto platformPortEntry = hw_->getPlatform()
                                 ->getPlatformPort(port->getID())
                                 ->getPlatformPortEntry();
    if (!platformPortEntry) {
      throw FbossError(
          "Port: ",
          port->getName(),
          " doesn't have PlatformPortEntry. Not allowed to use flex port api");
    }

    bcm_port_resource_t newPortRes;
    bcm_port_resource_t_init(&newPortRes);
    newPortRes.port = port->getID();

    const auto& iphyLanes = utility::getOrderedIphyLanes(
        *platformPortEntry, chips, port->getProfileID());
    newPortRes.physical_port =
        basePhysicalPort_ + (iphyLanes[0].lane - basePhysicalLane_);

    newPortRes.lanes = desiredLaneMode_;
    newPortRes.speed = static_cast<int>((*profileConf).speed);
    newPortRes.fec_type =
        utility::phyFecModeToBcmPortPhyFec((*profileConf).iphy.fec);
    newPortRes.link_training = 0; /* turn off link training as default */
    // set lane_config as default and then will let BcmPort::setPortResource
    // program the correct config later when it get the medium type and
    // modulation.
    newPortRes.phy_lane_config = -1;
    ports_.push_back(newPortRes);
  }

  numAddedPorts_ = filteredPorts.size();
  return filteredPorts;
}

void BcmPortResourceBuilder::program() {
  if (ports_.empty()) {
    throw FbossError(
        "Can't program the portresource build since the final array is empty");
  }

  XLOG(INFO) << "About to reconfigure port group of for control port: "
             << controllingPort_->getPortID()
             << ", from port size: " << numRemovedPorts_
             << " to port size: " << numAddedPorts_;

  for (const auto& portRes : ports_) {
    XLOG(INFO) << "Set port resource. logical_id:" << portRes.port
               << ", physical_port:" << portRes.physical_port
               << ", lanes:" << portRes.lanes;
  }

  // Use Flex Port Api to program the port resources list
  auto rv = bcm_port_resource_multi_set(
      hw_->getUnit(), numRemovedPorts_ + numAddedPorts_, ports_.data());
  bcmCheckError(
      rv,
      "Fail to configure multiple port resources for controlling port: ",
      controllingPort_->getBcmGport(),
      ", remove port size: ",
      numRemovedPorts_,
      ", newly add port size: ",
      numAddedPorts_);
}

} // namespace facebook::fboss
