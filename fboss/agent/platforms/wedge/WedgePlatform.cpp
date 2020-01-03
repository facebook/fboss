/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgePlatform.h"

#include <folly/Memory.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/wedge/WedgePort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/usb/UsbError.h"
#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include <future>

DEFINE_string(
    volatile_state_dir,
    "/dev/shm/fboss",
    "Directory for storing volatile state");
DEFINE_string(
    persistent_state_dir,
    "/var/facebook/fboss",
    "Directory for storing persistent state");

using folly::MacAddress;
using std::make_unique;
using std::string;

namespace {
constexpr auto kNumWedge40Qsfps = 16;
}

namespace facebook::fboss {

WedgePlatform::WedgePlatform(std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmPlatform(std::move(productInfo)),
      qsfpCache_(std::make_unique<AutoInitQsfpCache>()) {}

void WedgePlatform::initImpl() {
  BcmAPI::init(loadConfig());
  hw_.reset(new BcmSwitch(this));
}

WedgePlatform::~WedgePlatform() {}

void WedgePlatform::initPorts() {
  portMapping_ = createPortMapping();
}

WedgePlatform::BcmPlatformPortMap WedgePlatform::getPlatformPortMap() {
  if (portMapping_ == nullptr) {
    throw FbossError(
        "Platform port mapping is empty, need to initPorts() first");
  }

  BcmPlatformPortMap mapping;
  for (const auto& kv : *portMapping_) {
    opennsl_port_t id = kv.first;
    mapping[id] = kv.second.get();
  }
  return mapping;
}

void WedgePlatform::stop() {
  // destroying the cache will cause it to stop the QsfpCacheThread
  qsfpCache_.reset();
}

void WedgePlatform::onHwInitialized(SwSwitch* sw) {
  // could populate with initial ports here, but should get taken care
  // of through state changes sent to the stateUpdated method.
  initLEDs();
  // Make sure the initial status of the LEDs reflects the current port
  // settings.
  for (const auto& entry : *portMapping_) {
    // As some platform allows add/remove port at the first time applying the
    // config, we should skip those ports which doesn't exist in hw.
    if (!hw_->getPortTable()->getBcmPortIf(entry.first)) {
      continue;
    }
    bool up = hw_->isPortUp(entry.first);
    entry.second->linkStatusChanged(up, true);
  }
  sw->registerStateObserver(this, "WedgePlatform");
}

void WedgePlatform::stateUpdated(const StateDelta& delta) {
  QsfpCache::PortMapThrift changedPorts;
  auto portsDelta = delta.getPortsDelta();
  for (const auto& entry : portsDelta) {
    auto newPort = entry.getNew();
    if (newPort) {
      auto platformPort = getPort(newPort->getID());
      if (platformPort->supportsTransceiver()) {
        changedPorts[newPort->getID()] = platformPort->toThrift(newPort);
      }
    }
  }
  qsfpCache_->portsChanged(changedPorts);
}

HwSwitch* WedgePlatform::getHwSwitch() const {
  return hw_.get();
}

string WedgePlatform::getVolatileStateDir() const {
  return FLAGS_volatile_state_dir;
}

string WedgePlatform::getPersistentStateDir() const {
  return FLAGS_persistent_state_dir;
}

void WedgePlatform::onUnitCreate(int unit) {
  warmBootHelper_ =
      std::make_unique<DiscBackedBcmWarmBootHelper>(unit, getWarmBootDir());
  dumpHwConfig();
}

void WedgePlatform::onUnitAttach(int /*unit*/) {}

std::unique_ptr<BaseWedgeI2CBus> WedgePlatform::getI2CBus() {
  return make_unique<WedgeI2CBus>();
}

TransceiverIdxThrift WedgePlatform::getPortMapping(PortID portId) const {
  return getPort(portId)->getTransceiverMapping();
}

WedgePort* WedgePlatform::getPort(PortID id) const {
  return portMapping_->getPort(id);
}
WedgePort* WedgePlatform::getPort(TransceiverID id) const {
  return portMapping_->getPort(id);
}
PlatformPort* WedgePlatform::getPlatformPort(const PortID port) const {
  return getPort(port);
}

} // namespace facebook::fboss
