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

#include <folly/Memory.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/sim/SimHandler.h"
#include "fboss/agent/hw/sim/SimSwitch.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"

DEFINE_string(
    volatile_state_dir,
    "/tmp/fboss_sim/volatile",
    "Directory for storing volatile state");
DEFINE_string(
    persistent_state_dir,
    "/tmp/fboss_sim/persistent",
    "Directory for storing persistent state");

using std::make_unique;
using std::unique_ptr;

namespace facebook::fboss {

SimPlatform::SimPlatform(folly::MacAddress mac, uint32_t numPorts)
    : Platform(nullptr), mac_(mac), hw_(new SimSwitch(this, numPorts)) {}

SimPlatform::~SimPlatform() {}

HwSwitch* SimPlatform::getHwSwitch() const {
  return hw_.get();
}

void SimPlatform::onHwInitialized(SwSwitch* /*sw*/) {}

void SimPlatform::onInitialConfigApplied(SwSwitch* /*sw*/) {}

void SimPlatform::stop() {}

unique_ptr<ThriftHandler> SimPlatform::createHandler(SwSwitch* sw) {
  SimSwitch* sim = static_cast<SimSwitch*>(sw->getHw());
  return std::make_unique<SimHandler>(sw, sim);
}

std::string SimPlatform::getVolatileStateDir() const {
  return FLAGS_volatile_state_dir;
}

std::string SimPlatform::getPersistentStateDir() const {
  return FLAGS_persistent_state_dir;
}

void SimPlatform::initPorts() {}

} // namespace facebook::fboss
