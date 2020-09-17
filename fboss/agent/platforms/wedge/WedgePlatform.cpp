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
#include "fboss/agent/hw/bcm/BcmConfig.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/wedge/WedgePort.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
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

DEFINE_string(
    fabric_location,
    "",
    "Provides location of fabric , LEFT or RIGHT");

using folly::MacAddress;
using std::make_unique;
using std::string;

namespace {
constexpr auto kNumWedge40Qsfps = 16;
} // namespace

namespace facebook::fboss {

WedgePlatform::WedgePlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping)
    : BcmPlatform(std::move(productInfo), std::move(platformMapping)),
      qsfpCache_(std::make_unique<AutoInitQsfpCache>()) {}

void WedgePlatform::initImpl(uint32_t hwFeaturesDesired) {
  if (getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    BcmAPI::initHSDK(loadYamlConfig());
  } else {
    BcmAPI::init(loadConfig());
  }
  hw_.reset(new BcmSwitch(this, hwFeaturesDesired));
}

WedgePlatform::~WedgePlatform() {
  BcmAPI::shutdown();
}

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
    bcm_port_t id = kv.first;
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
  updatePorts(delta);
  updateQsfpCache(delta);
}

void WedgePlatform::updateQsfpCache(const StateDelta& delta) {
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

void WedgePlatform::updatePorts(const StateDelta& delta) {
  for (const auto& entry : delta.getPortsDelta()) {
    const auto newPort = entry.getNew();
    if (newPort) {
      getPort(newPort->getID())->portChanged(newPort);
    }
  }
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
  warmBootHelper_ = std::make_unique<BcmWarmBootHelper>(unit, getWarmBootDir());
  dumpHwConfig();
}

void WedgePlatform::onUnitAttach(int /*unit*/) {}

void WedgePlatform::preWarmbootStateApplied() {
  // Update QsfpCache with the existing ports
  QsfpCache::PortMapThrift changedPorts;
  for (const auto& entry : *portMapping_) {
    // As some platform allows add/remove port at the first time applying the
    // config, we should skip those ports which doesn't exist in hw.
    auto bcmPortIf = hw_->getPortTable()->getBcmPortIf(entry.first);
    if (!bcmPortIf) {
      XLOG(WARNING) << "Port:" << entry.first << " is not in hw port table.";
      continue;
    }

    if (entry.second->supportsTransceiver()) {
      PortStatus s;
      *s.enabled_ref() = bcmPortIf->isEnabled();
      *s.up_ref() = bcmPortIf->isUp();
      *s.speedMbps_ref() = static_cast<int>(bcmPortIf->getSpeed());
      s.transceiverIdx_ref() = entry.second->getTransceiverMapping();
      changedPorts[entry.first] = s;
    }
  }
  XLOG(INFO) << "[preWarmbootStateApplied]: Will update " << changedPorts.size()
             << " ports to qsfp cache.";
  qsfpCache_->portsChanged(changedPorts);
}

std::unique_ptr<BaseWedgeI2CBus> WedgePlatform::getI2CBus() {
  return make_unique<WedgeI2CBus>();
}

TransceiverIdxThrift WedgePlatform::getPortMapping(
    PortID portId,
    cfg::PortSpeed /* speed */) const {
  return getPort(portId)->getTransceiverMapping();
}

WedgePort* WedgePlatform::getPort(PortID id) const {
  return portMapping_->getPort(id);
}
std::vector<WedgePort*> WedgePlatform::getPortsByTransceiverID(
    TransceiverID id) const {
  return portMapping_->getPortsByTransceiverID(id);
}
PlatformPort* WedgePlatform::getPlatformPort(const PortID port) const {
  return getPort(port);
}

std::map<std::string, std::string> WedgePlatform::loadConfig() {
  auto cfg = config();
  if (cfg) {
    return *cfg->thrift.platform_ref()->chip_ref()->get_bcm().config_ref();
  }
  return BcmConfig::loadDefaultConfig();
}

std::string WedgePlatform::loadYamlConfig() {
  auto cfg = config();
  if (!cfg) {
    // TODO(josep5wu) Need to revisit whether we should generate a default
    // yaml config just like WedgePlatform::loadConfig()
    throw FbossError("Failed to get agent config");
  }
  if (auto yamlConfig =
          cfg->thrift.platform_ref()->chip_ref()->get_bcm().yamlConfig_ref()) {
    return *yamlConfig;
  }
  throw FbossError("Failed to get bcm yaml config from agent config");
}
} // namespace facebook::fboss
