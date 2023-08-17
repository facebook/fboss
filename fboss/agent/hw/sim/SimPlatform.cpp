/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sim/SimPlatform.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/sim/SimPlatformMapping.h"
#include "fboss/agent/hw/sim/SimPlatformPort.h"
#include "fboss/agent/hw/sim/SimSwitch.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include <folly/Memory.h>

using std::make_unique;
using std::unique_ptr;

namespace facebook::fboss {

SimPlatform::SimPlatform(folly::MacAddress mac, uint32_t numPorts)
    : Platform(nullptr, std::make_unique<SimPlatformMapping>(numPorts), mac),
      hw_(new SimSwitch(this, numPorts)),
      numPorts_(numPorts),
      agentDirUtil_(new AgentDirectoryUtil(
          "/tmp/fboss_sim/volatile",
          "/tmp/fboss_sim/persistent")) {
  initPorts();
}

SimPlatform::~SimPlatform() {}

HwSwitch* SimPlatform::getHwSwitch() const {
  return hw_.get();
}

void SimPlatform::onHwInitialized(HwSwitchCallback* /*sw*/) {}

void SimPlatform::onInitialConfigApplied(HwSwitchCallback* /*sw*/) {}

void SimPlatform::stop() {}

std::string SimPlatform::getVolatileStateDir() const {
  FLAGS_volatile_state_dir = "/tmp/fboss_sim/volatile";
  return FLAGS_volatile_state_dir;
}

std::string SimPlatform::getPersistentStateDir() const {
  FLAGS_persistent_state_dir = "/tmp/fboss_sim/persistent";
  return FLAGS_persistent_state_dir;
}

void SimPlatform::initPorts() {
  for (auto i = 0; i < numPorts_; i++) {
    auto portID = PortID(i);
    portMapping_.emplace(
        portID, std::make_unique<SimPlatformPort>(portID, this));
  }
}

PlatformPort* SimPlatform::getPlatformPort(PortID id) const {
  if (auto port = portMapping_.find(id); port != portMapping_.end()) {
    return port->second.get();
  }
  throw FbossError("Can't find SimPlatform PlatformPort for ", id);
}

} // namespace facebook::fboss
