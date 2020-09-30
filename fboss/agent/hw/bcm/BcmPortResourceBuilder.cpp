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
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmPortUtils.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

extern "C" {
#include <bcm/switch.h>
}

namespace {
const int kBcmL2DeleteStatic = 0x1;
} // namespace

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
  if (!portResources_.empty()) {
    throw FbossError(
        "All the removed ports should be in front of the final array. ",
        "Current array size:",
        portResources_.size());
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
    portResources_.push_back(oldPortRes);
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

    const auto& itProfile = (*platformPortEntry)
                                .supportedProfiles_ref()
                                ->find(port->getProfileID());
    if (itProfile != (*platformPortEntry).supportedProfiles_ref()->end()) {
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
          *(*platformPortEntry).mapping_ref()->name_ref(),
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

int BcmPortResourceBuilder::getBaseLane(std::shared_ptr<Port> port) {
  auto platformPortEntry = hw_->getPlatform()
                               ->getPlatformPort(port->getID())
                               ->getPlatformPortEntry();
  if (!platformPortEntry) {
    throw FbossError(
        "Port: ",
        port->getID(),
        " doesn't have PlatformPortEntry. Not allowed to use flex port api");
  }
  const auto& iphyLanes = utility::getOrderedIphyLanes(
      *platformPortEntry,
      hw_->getPlatform()->getDataPlanePhyChips(),
      port->getProfileID());
  return *iphyLanes[0].lane_ref();
}

std::vector<std::shared_ptr<Port>> BcmPortResourceBuilder::addPorts(
    const std::vector<std::shared_ptr<Port>>& ports) {
  // BRCM SDK limits all the remove ports should be in front of the final array
  if (portResources_.empty()) {
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
    auto baseLane = getBaseLane(port);
    if (basePhysicalLane_ == -1 || basePhysicalLane_ > baseLane) {
      basePhysicalLane_ = baseLane;
    }
  }

  for (auto port : filteredPorts) {
    auto profileConf =
        hw_->getPlatform()->getPortProfileConfig(port->getProfileID());
    auto platformPort = hw_->getPlatform()->getPlatformPort(port->getID());
    if (!profileConf) {
      throw FbossError(
          "Port: ",
          port->getName(),
          " demands unsupported profile: ",
          apache::thrift::util::enumNameSafe(port->getProfileID()));
    }
    bcm_port_resource_t newPortRes;
    bcm_port_resource_t_init(&newPortRes);
    newPortRes.port = port->getID();

    const auto& baseIphyLane = getBaseLane(port);
    newPortRes.physical_port =
        basePhysicalPort_ + (baseIphyLane - basePhysicalLane_);

    newPortRes.lanes = desiredLaneMode_;
    newPortRes.speed = static_cast<int>(*(*profileConf).speed_ref());
    newPortRes.fec_type = utility::phyFecModeToBcmPortPhyFec(
        platformPort->shouldDisableFEC() ? phy::FecMode::NONE
                                         : profileConf->get_iphy().get_fec());
    newPortRes.phy_lane_config = utility::getDesiredPhyLaneConfig(*profileConf);
    newPortRes.link_training = 0; /* turn off link training as default */
    portResources_.push_back(newPortRes);
  }

  numAddedPorts_ = filteredPorts.size();
  return filteredPorts;
}

// Some *_switch_control_set operations are performed on a port-by-port basis
// These controls are not updated by the flexport API, so we need to disable
// these controls before changing port groups, and then re-enable them after
void BcmPortResourceBuilder::setPortSpecificControls(
    bcm_port_t bcmPort,
    bool enable) {
  auto enableStr = enable ? "enable" : "disable";
  auto enableInt = enable ? 1 : 0;
  int unit_ = hw_->getUnit();
  int rv = bcm_switch_control_port_set(
      unit_, bcmPort, bcmSwitchArpRequestToCpu, enableInt);
  bcmCheckError(
      rv, "failed to ", enableStr, " ARP request trapping for port ", bcmPort);

  rv = bcm_switch_control_port_set(
      unit_, bcmPort, bcmSwitchArpReplyToCpu, enableInt);
  bcmCheckError(
      rv, "failed to ", enableStr, " ARP reply trapping for port", bcmPort);

  rv = bcm_switch_control_port_set(
      unit_, bcmPort, bcmSwitchDhcpPktDrop, enableInt);
  bcmCheckError(
      rv, "failed to ", enableStr, " DHCP dropping for port ", bcmPort);

  rv = bcm_switch_control_port_set(
      unit_, bcmPort, bcmSwitchDhcpPktToCpu, enableInt);
  bcmCheckError(
      rv, "failed to ", enableStr, " DHCP request trapping for port ", bcmPort);

  rv = bcm_switch_control_port_set(
      unit_, bcmPort, bcmSwitchNdPktToCpu, enableInt);
  bcmCheckError(rv, "failed to ", enableStr, " ND trapping for port ", bcmPort);
}

void BcmPortResourceBuilder::program() {
  if (portResources_.empty()) {
    throw FbossError(
        "Can't program the portresource build since the final array is empty");
  }
  int unit_ = hw_->getUnit();

  XLOG(INFO) << "About to reconfigure port group of for control port: "
             << controllingPort_->getPortID()
             << ", from port size: " << numRemovedPorts_
             << " to port size: " << numAddedPorts_;

  std::vector<bcm_port_t> oldPortIDs;
  std::vector<bcm_port_t> newPortIDs;
  auto i = 0;
  for (const auto& portRes : portResources_) {
    XLOG(INFO) << "Set port resource. logical_id:" << portRes.port
               << ", physical_port:" << portRes.physical_port
               << ", lanes:" << portRes.lanes;
    if (i < numRemovedPorts_) {
      oldPortIDs.push_back(portRes.port);
    } else {
      newPortIDs.push_back(portRes.port);
    }
    i++;
  }

  // The flexport API requires us to do the following for all ports:
  // * remove any l2 forwarding entries for the port
  // * disable any switch_control that may be set for the port
  for (auto port : oldPortIDs) {
    int rv = bcm_l2_addr_delete_by_port(unit_, -1, port, kBcmL2DeleteStatic);
    bcmCheckError(
        rv,
        "failed to delete static + non-static l2 entries for bcmPort ",
        port);

    setPortSpecificControls(port, false);
  }

  // Use Flex Port Api to program the port resources list
  auto rv = bcm_port_resource_multi_set(
      hw_->getUnit(), numRemovedPorts_ + numAddedPorts_, portResources_.data());
  bcmCheckError(
      rv,
      "Fail to configure multiple port resources for controlling port: ",
      controllingPort_->getBcmGport(),
      ", remove port size: ",
      numRemovedPorts_,
      ", newly add port size: ",
      numAddedPorts_);

  // Enable any per-port switch_controls that we previously cleared
  for (auto port : newPortIDs) {
    bcm_port_resource_t resource;
    bcm_port_resource_get(unit_, port, &resource);
    setPortSpecificControls(port, true);
  }
}

} // namespace facebook::fboss
