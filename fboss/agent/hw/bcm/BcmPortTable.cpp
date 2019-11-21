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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPlatformPort.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortGroup.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <folly/Memory.h>

extern "C" {
#include <opennsl/port.h>
}

namespace facebook {
namespace fboss {

using std::make_pair;
using std::make_unique;
using std::unique_ptr;

BcmPortTable::BcmPortTable(BcmSwitch* hw) : hw_(hw) {}

BcmPortTable::~BcmPortTable() {}

void BcmPortTable::addBcmPort(opennsl_port_t logicalPort, bool warmBoot) {
  // Find the platform port object
  BcmPlatformPort* platformPort = dynamic_cast<BcmPlatformPort*>(
      hw_->getPlatform()->getPlatformPort(PortID(logicalPort)));
  if (platformPort == nullptr) {
    throw FbossError("Can't find platform port for port:", logicalPort);
  }

  // Create a BcmPort object
  PortID fbossPortID = platformPort->getPortID();
  auto bcmPort = make_unique<BcmPort>(hw_, logicalPort, platformPort);
  platformPort->setBcmPort(bcmPort.get());
  bcmPort->init(warmBoot);

  fbossPhysicalPorts_[fbossPortID] = bcmPort.get();
  bcmPhysicalPorts_[logicalPort] = std::move(bcmPort);
}

void BcmPortTable::initPorts(
    const opennsl_port_config_t* portConfig,
    bool warmBoot) {
  // Ask the platform for the list of ports on this platform,
  // and then associate the BcmPort and BcmPlatformPort objects.
  //
  // Note that we only create BcmPort objects for ports defined by the
  // platform.  For instance, even though the Trident2 chip may support up to
  // 128 ports, if the platform only defines 32 ports we will only create 32
  // BcmPort objects.
  for (const auto& entry : hw_->getPlatform()->getPlatformPortMap()) {
    opennsl_port_t bcmPortNum = entry.first;
    BcmPlatformPort* platPort = entry.second;

    // If the port doesn't support add or remove port, make sure this port
    // number actually exists on the switch hardware
    if (!OPENNSL_PBMP_MEMBER(portConfig->port, bcmPortNum)) {
      // If the platform support add or remove port, we can skip for now.
      // And we'll add or remove ports when we apply agent config for the first
      // time
      if (platPort->supportsAddRemovePort()) {
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

BcmPort* BcmPortTable::getBcmPort(opennsl_port_t id) const {
  auto iter = bcmPhysicalPorts_.find(id);
  if (iter == bcmPhysicalPorts_.end()) {
    throw FbossError("Cannot find the BCM port object for BCM port ", id);
  }
  return iter->second.get();
}

BcmPort* BcmPortTable::getBcmPortIf(opennsl_port_t id) const {
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

void BcmPortTable::forFilteredEach(Filter predicate, FilterAction action)
    const {
  auto iterator = FilterIterator(fbossPhysicalPorts_, predicate);
  std::for_each(iterator.begin(), iterator.end(), action);
}
} // namespace fboss
} // namespace facebook
