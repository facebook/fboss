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

namespace facebook { namespace fboss {

using std::make_unique;
using std::unique_ptr;
using std::make_pair;

BcmPortTable::BcmPortTable(BcmSwitch *hw)
  : hw_(hw) {
}

BcmPortTable::~BcmPortTable() {
}

void BcmPortTable::initPorts(const opennsl_port_config_t* portConfig,
                             bool warmBoot) {
  // Ask the platform for the list of ports on this platform,
  // and then associate the BcmPort and BcmPlatformPort objects.
  //
  // Note that we only create BcmPort objects for ports defined by the
  // platform.  For instance, even though the Trident2 chip may support up to
  // 128 ports, if the platform only defines 32 ports we will only create 32
  // BcmPort objects.
  auto platformPorts = hw_->getPlatform()->initPorts();
  for (const auto& entry : platformPorts) {
    opennsl_port_t bcmPortNum = entry.first;
    BcmPlatformPort* platPort = entry.second;

    // Make sure this port number actually exists on the switch hardware
    if (!OPENNSL_PBMP_MEMBER(portConfig->port, bcmPortNum)) {
      throw FbossError("platform attempted to initialize BCM port ",
                       bcmPortNum, " which does not exist");
    }

    // Create a BcmPort object
    PortID fbossPortID = platPort->getPortID();
    auto bcmPort = make_unique<BcmPort>(hw_, bcmPortNum, platPort);
    platPort->setBcmPort(bcmPort.get());
    bcmPort->init(warmBoot);

    fbossPhysicalPorts_.emplace(fbossPortID, bcmPort.get());
    bcmPhysicalPorts_.emplace(bcmPortNum, std::move(bcmPort));
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

}} // namespace facebook::fboss
