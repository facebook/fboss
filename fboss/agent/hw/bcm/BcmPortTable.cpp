/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPortTable.h"

#include "common/stats/MonotonicCounter.h"
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/Memory.h>
#include <folly/logging/xlog.h>

extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss {

using std::make_pair;
using std::make_unique;
using std::unique_ptr;

BcmPortTable::BcmPortTable(BcmSwitch* hw) : hw_(hw) {}

BcmPortTable::~BcmPortTable() {}

void BcmPortTable::addBcmPort(bcm_port_t logicalPort, bool warmBoot) {
  // Find the platform port object
  BcmPlatformPort* platformPort = dynamic_cast<BcmPlatformPort*>(
      hw_->getPlatform()->getPlatformPort(PortID(logicalPort)));
  if (platformPort == nullptr) {
    throw FbossError("Can't find platform port for port: ", logicalPort);
  }

  // Create a BcmPort object
  PortID fbossPortID = platformPort->getPortID();
  auto bcmPort = make_unique<BcmPort>(hw_, logicalPort, platformPort);
  platformPort->setBcmPort(bcmPort.get());
  bcmPort->init(warmBoot);

  auto it = bcmPhysicalPorts_.find(logicalPort);
  if (it != bcmPhysicalPorts_.end()) {
    throw FbossError(
        "Cannot call addBcmPort on a port that already exists: ", logicalPort);
  }

  fbossPhysicalPorts_.insert(fbossPortID, bcmPort.get());
  bcmPhysicalPorts_.insert(logicalPort, std::move(bcmPort));
}

void BcmPortTable::removeBcmPort(bcm_port_t logicalPort) {
  // Find the platform port object
  BcmPlatformPort* platformPort = dynamic_cast<BcmPlatformPort*>(
      hw_->getPlatform()->getPlatformPort(PortID(logicalPort)));
  if (platformPort == nullptr) {
    throw FbossError("Can't find platform port for port:", logicalPort);
  }

  auto it = bcmPhysicalPorts_.find(logicalPort);
  if (it == bcmPhysicalPorts_.end()) {
    throw FbossError(
        "Cannot call removeBcmPort for a port that doesn't exist: ",
        logicalPort);
  }

  PortID fbossPortID = platformPort->getPortID();
  // folly's concurrent hashmaps are implemented using hazard pointers that
  // are only cleared after reaching a certain memory threshold. We need to
  // explicitly clean up the bcmPort object now so it's not still in use when
  // we configure dependent components of the system.
  it->second->destroy();
  fbossPhysicalPorts_.erase(fbossPortID);
  bcmPhysicalPorts_.erase(logicalPort);
}

void BcmPortTable::initPorts(
    const bcm_port_config_t* portConfig,
    bool warmBoot) {
  // Ask the platform for the list of ports on this platform,
  // and then associate the BcmPort and BcmPlatformPort objects.
  //
  // Note that we only create BcmPort objects for ports defined by the
  // platform.  For instance, even though the Trident2 chip may support up to
  // 128 ports, if the platform only defines 32 ports we will only create 32
  // BcmPort objects.
  for (const auto& entry : hw_->getPlatform()->getPlatformPortMap()) {
    bcm_port_t bcmPortNum = entry.first;

    // If the port doesn't support add or remove port, make sure this port
    // number actually exists on the switch hardware
    if (!BCM_PBMP_MEMBER(portConfig->port, bcmPortNum)) {
      // If the platform support add or remove port, we can skip for now.
      // And we'll add or remove ports when we apply agent config for the first
      // time
      if (hw_->getPlatform()->supportsAddRemovePort()) {
        continue;
      }
      throw FbossError(
          "platform attempted to initialize BCM port ",
          bcmPortNum,
          " which does not exist in hardware");
    }

    addBcmPort(bcmPortNum, warmBoot);
  }

  initPortGroups();
}

BcmPort* BcmPortTable::getBcmPort(bcm_port_t id) const {
  auto iter = bcmPhysicalPorts_.find(id);
  if (iter == bcmPhysicalPorts_.end()) {
    throw FbossError("Cannot find the BCM port object for BCM port ", id);
  }
  return iter->second.get();
}

BcmPort* BcmPortTable::getBcmPortIf(bcm_port_t id) const {
  auto iter = bcmPhysicalPorts_.find(id);
  if (iter == bcmPhysicalPorts_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

BcmPort* BcmPortTable::getBcmPort(PortID id) const {
  auto iter = fbossPhysicalPorts_.find(id);
  if (iter == fbossPhysicalPorts_.end()) {
    throw FbossError("Cannot find the BCM port object for FBOSS port ID ", id);
  }
  return iter->second;
}

BcmPort* BcmPortTable::getBcmPortIf(PortID id) const {
  auto iter = fbossPhysicalPorts_.find(id);
  if (iter == fbossPhysicalPorts_.end()) {
    return nullptr;
  }
  return iter->second;
}

void BcmPortTable::updatePortStats() {
  for (const auto& entry : bcmPhysicalPorts_) {
    BcmPort* bcmPort = entry.second.get();
    bcmPort->updateStats();
  }
}

std::map<PortID, phy::PhyInfo> BcmPortTable::updateIPhyInfo() const {
  std::map<PortID, phy::PhyInfo> phyInfo;
  for (const auto& entry : bcmPhysicalPorts_) {
    BcmPort* bcmPort = entry.second.get();
    phyInfo[getPortId(entry.first)] = bcmPort->updateIPhyInfo();
  }
  return phyInfo;
};

void BcmPortTable::initPortGroups() {
  auto subsidiaryPortsMap =
      utility::getSubsidiaryPortIDs(hw_->getPlatform()->getPlatformPorts());

  for (auto& entry : bcmPhysicalPorts_) {
    BcmPort* bcmPort = entry.second.get();
    if (bcmPort->getPortGroup()) {
      // if port is part of a group already skip it.
      continue;
    }

    // The existing design is based on the assumption that port group == core.
    // This works on TD2/TH, which a core has 4 lanes, and can have up to 4
    // logical ports. But for TH3 Blackhawk core or future more powerful cores,
    // a core can have 8 lanes and can have up to 8 logical ports. And the
    // portgroup also depends on the hardware design. For example, for Wedge400,
    // we split one core into two front panel ports, and each of these front
    // panel ports can break into up to 4 logical ports. If we use
    // `bcm_port_subsidiary_ports_get` to get all the subsidiary ports, we'll
    // get 7 ports. i.e. eth1/17/1 [logical port: 20-23], eth1/18/1 [24-27],
    // if we pass logical port id:20, we'll get 21/22/23/24/25/26/27 back, which
    // is not we want for PortGroup, as we still want eth1/17/1 as one group,
    // while eth1/18/1 is another.
    // Hence, we introduced the recent PlatformPort refactor to use config to
    // tell us which ports are expected to be in the same group.
    // If we can't get subsidiary ports map from config, we fall back to use
    // the old logic to get port group.
    subsidiaryPortsMap.empty()
        ? initPortGroupLegacy(bcmPort)
        : initPortGroupFromConfig(bcmPort, subsidiaryPortsMap);
  }
}

void BcmPortTable::initPortGroupLegacy(BcmPort* controllingPort) {
  bcm_pbmp_t pbmp;
  auto rv = bcm_port_subsidiary_ports_get(
      hw_->getUnit(), controllingPort->getBcmPortId(), &pbmp);
  if (rv == BCM_E_PORT) {
    // not a controlling port, skip
    return;
  } else if (rv != BCM_E_NONE) {
    bcmLogFatal(
        rv,
        hw_,
        "Failed to get subsidiary ports for port ",
        controllingPort->getBcmPortId());
  } else {
    std::vector<BcmPort*> subsidiaryPorts;
    bcm_port_t subsidiaryPort;
    BCM_PBMP_ITER(pbmp, subsidiaryPort) {
      subsidiaryPorts.push_back(getBcmPort(subsidiaryPort));
    }

    // Note that the subsidiary_ports pbmp includes the controlling port, so
    // we don't need to add the controlling port manually
    auto group = std::make_unique<BcmPortGroup>(
        hw_, controllingPort, std::move(subsidiaryPorts));

    auto members = group->allPorts();
    for (auto& member : members) {
      member->registerInPortGroup(group.get());
    }
    bcmPortGroups_.push_back(std::move(group));
  }
}

void BcmPortTable::initPortGroupFromConfig(
    BcmPort* controllingPort,
    const std::map<PortID, std::vector<PortID>>& subsidiaryPortsMap) {
  auto itSubsidiaryPorts =
      subsidiaryPortsMap.find(controllingPort->getPortID());
  if (itSubsidiaryPorts == subsidiaryPortsMap.end()) {
    // not a controlling port, skip
    return;
  }
  std::vector<BcmPort*> subsidiaryPorts;
  for (auto portId : itSubsidiaryPorts->second) {
    // only put the BcmPort pointer to the group if the port exists in
    // the hardware. With the new flex port features on TH3 allowing us to
    // add and remove port, we might have some PlatformPort not created in
    // bcm yet.
    // TODO(joseph5wu): We might also need to add some check to make sure
    // the ports in config is allowed to be in the same group.
    if (auto* subBcmPort = getBcmPortIf(portId)) {
      subsidiaryPorts.push_back(subBcmPort);
    }
  }
  auto group = std::make_unique<BcmPortGroup>(
      hw_, controllingPort, std::move(subsidiaryPorts));

  auto members = group->allPorts();
  for (auto& member : members) {
    member->registerInPortGroup(group.get());
  }
  bcmPortGroups_.push_back(std::move(group));
}
} // namespace facebook::fboss
