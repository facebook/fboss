/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiBcmGalaxyPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge100PlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge40PlatformPort.h"
#include "fboss/agent/platforms/sai/SaiFakePlatformPort.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPlatformPort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include "fboss/agent/hw/sai/switch/SaiHandler.h"

DEFINE_string(
    hw_config_file,
    "hw_config",
    "File for dumping HW config on startup");

namespace {

std::unordered_map<std::string, std::string> kSaiProfileValues;

const char* saiProfileGetValue(
    sai_switch_profile_id_t /*profile_id*/,
    const char* variable) {
  auto saiProfileValItr = kSaiProfileValues.find(variable);
  return saiProfileValItr != kSaiProfileValues.end()
      ? saiProfileValItr->second.c_str()
      : nullptr;
}

int saiProfileGetNextValue(
    sai_switch_profile_id_t /* profile_id */,
    const char** variable,
    const char** value) {
  static auto saiProfileValItr = kSaiProfileValues.begin();
  if (!value) {
    saiProfileValItr = kSaiProfileValues.begin();
    return 0;
  }
  if (saiProfileValItr == kSaiProfileValues.end()) {
    return -1;
  }
  *variable = saiProfileValItr->first.c_str();
  *value = saiProfileValItr->second.c_str();
  ++saiProfileValItr;
  return 0;
}

sai_service_method_table_t kSaiServiceMethodTable = {
    .profile_get_value = saiProfileGetValue,
    .profile_get_next_value = saiProfileGetNextValue,
};

} // namespace

namespace facebook::fboss {

SaiPlatform::SaiPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping)
    : Platform(std::move(productInfo), std::move(platformMapping)),
      qsfpCache_(std::make_unique<AutoInitQsfpCache>()) {}

SaiPlatform::~SaiPlatform() {}

HwSwitch* SaiPlatform::getHwSwitch() const {
  return saiSwitch_.get();
}

void SaiPlatform::onHwInitialized(SwSwitch* sw) {
  initLEDs();
  sw->registerStateObserver(this, "SaiPlatform");
}

void SaiPlatform::updateQsfpCache(const StateDelta& delta) {
  QsfpCache::PortMapThrift changedPorts;
  auto portsDelta = delta.getPortsDelta();
  for (const auto& entry : portsDelta) {
    auto port = entry.getNew();
    if (port) {
      auto platformPort = getPort(port->getID());
      PortStatus portStatus;
      *portStatus.enabled_ref() = port->isEnabled();
      *portStatus.up_ref() = port->isUp();
      *portStatus.speedMbps_ref() = static_cast<int64_t>(port->getSpeed());
      portStatus.transceiverIdx_ref() =
          platformPort->getTransceiverMapping(port->getSpeed());
      changedPorts.insert(std::make_pair(port->getID(), portStatus));
    }
  }
  qsfpCache_->portsChanged(changedPorts);
}

void SaiPlatform::stateUpdated(const StateDelta& delta) {
  updateQsfpCache(delta);
}

void SaiPlatform::onInitialConfigApplied(SwSwitch* /* sw */) {}

void SaiPlatform::stop() {}

std::unique_ptr<ThriftHandler> SaiPlatform::createHandler(SwSwitch* sw) {
  return std::make_unique<SaiHandler>(sw, saiSwitch_.get());
}

TransceiverIdxThrift SaiPlatform::getPortMapping(
    PortID portId,
    cfg::PortSpeed speed) const {
  return getPort(portId)->getTransceiverMapping(speed);
}

std::string SaiPlatform::getHwConfigDumpFile() {
  return getVolatileStateDir() + "/" + FLAGS_hw_config_file;
}

void SaiPlatform::generateHwConfigFile() {
  auto hwConfig = getHwConfig();
  if (!folly::writeFile(hwConfig, getHwConfigDumpFile().c_str())) {
    throw FbossError(errno, "failed to generate hw config file. write failed");
  }
}

void SaiPlatform::initSaiProfileValues() {
  kSaiProfileValues.insert(
      std::make_pair(SAI_KEY_INIT_CONFIG_FILE, getHwConfigDumpFile()));
  kSaiProfileValues.insert(std::make_pair(
      SAI_KEY_WARM_BOOT_READ_FILE, getWarmBootHelper()->warmBootDataPath()));
  kSaiProfileValues.insert(std::make_pair(
      SAI_KEY_WARM_BOOT_WRITE_FILE, getWarmBootHelper()->warmBootDataPath()));
  kSaiProfileValues.insert(std::make_pair(
      SAI_KEY_BOOT_TYPE, getWarmBootHelper()->canWarmBoot() ? "1" : "0"));
  kSaiProfileValues.insert(std::make_pair(SAI_KEY_BOOT_TYPE, "0"));
}

void SaiPlatform::initImpl(uint32_t hwFeaturesDesired) {
  initSaiProfileValues();
  saiSwitch_ = std::make_unique<SaiSwitch>(this, hwFeaturesDesired);
  generateHwConfigFile();
}

void SaiPlatform::initPorts() {
  auto platformMode = getMode();
  for (auto& port : getPlatformPorts()) {
    std::unique_ptr<SaiPlatformPort> saiPort;
    PortID portId(port.first);
    if (platformMode == PlatformMode::WEDGE400C) {
      saiPort = std::make_unique<SaiWedge400CPlatformPort>(portId, this);
    } else if (platformMode == PlatformMode::WEDGE) {
      saiPort = std::make_unique<SaiBcmWedge40PlatformPort>(portId, this);
    } else if (platformMode == PlatformMode::WEDGE100) {
      saiPort = std::make_unique<SaiBcmWedge100PlatformPort>(portId, this);
    } else if (
        platformMode == PlatformMode::GALAXY_LC ||
        platformMode == PlatformMode::GALAXY_FC) {
      saiPort = std::make_unique<SaiBcmGalaxyPlatformPort>(portId, this);
    } else {
      saiPort = std::make_unique<SaiFakePlatformPort>(portId, this);
    }
    portMapping_.insert(std::make_pair(portId, std::move(saiPort)));
  }
}

SaiPlatformPort* SaiPlatform::getPort(PortID id) const {
  auto saiPortIter = portMapping_.find(id);
  if (saiPortIter == portMapping_.end()) {
    throw FbossError("failed to find port: ", id);
  }
  return saiPortIter->second.get();
}

PlatformPort* SaiPlatform::getPlatformPort(PortID port) const {
  return getPort(port);
}

std::optional<std::string> SaiPlatform::getPlatformAttribute(
    cfg::PlatformAttributes platformAttribute) {
  auto& platform = *config()->thrift.platform_ref();

  if (auto platformSettings = platform.platformSettings_ref()) {
    auto platformIter = platformSettings->find(platformAttribute);
    if (platformIter == platformSettings->end()) {
      return std::nullopt;
    }
    return platformIter->second;
  } else {
    return std::nullopt;
  }
}

sai_service_method_table_t* SaiPlatform::getServiceMethodTable() const {
  return &kSaiServiceMethodTable;
}

HwSwitchWarmBootHelper* SaiPlatform::getWarmBootHelper() {
  if (!wbHelper_) {
    wbHelper_ = std::make_unique<HwSwitchWarmBootHelper>(
        0, getWarmBootDir(), "sai_adaptor_state_");
  }
  return wbHelper_.get();
}

QsfpCache* SaiPlatform::getQsfpCache() const {
  return qsfpCache_.get();
}

PortID SaiPlatform::findPortID(
    cfg::PortSpeed speed,
    std::vector<uint32_t> lanes) const {
  for (const auto& portMapping : portMapping_) {
    const auto& platformPort = portMapping.second;
    if (!platformPort->getProfileIDBySpeedIf(speed) ||
        platformPort->getHwPortLanes(speed) != lanes) {
      continue;
    }
    return platformPort->getPortID();
  }
  throw FbossError("platform port not found");
}

std::vector<phy::TxSettings> SaiPlatform::getPlatformPortTxSettings(
    PortID port,
    cfg::PortProfileID profile) {
  auto platformPort = getPlatformPort(port);
  CHECK(platformPort);
  const auto& iphyPinConfigs = platformPort->getIphyPinConfigs(profile);
  std::vector<phy::TxSettings> txSettings;
  for (auto& pinConfig : iphyPinConfigs) {
    auto tx = pinConfig.tx_ref();
    if (!tx) {
      continue;
    }
    txSettings.push_back(tx.value());
  }
  return txSettings;
}

std::vector<phy::RxSettings> SaiPlatform::getPlatformPortRxSettings(
    PortID port,
    cfg::PortProfileID profile) {
  auto platformPort = getPlatformPort(port);
  CHECK(platformPort);
  const auto& iphyPinConfigs = platformPort->getIphyPinConfigs(profile);
  std::vector<phy::RxSettings> rxSettings;
  for (auto& pinConfig : iphyPinConfigs) {
    auto rx = pinConfig.rx_ref();
    if (!rx) {
      continue;
    }
    rxSettings.push_back(rx.value());
  }
  return rxSettings;
}

std::vector<SaiPlatformPort*> SaiPlatform::getPortsWithTransceiverID(
    TransceiverID id) const {
  std::vector<SaiPlatformPort*> ports;
  for (const auto& port : portMapping_) {
    if (auto tcvrID = port.second->getTransceiverID()) {
      if (tcvrID == id &&
          port.second->getCurrentProfile() !=
              cfg::PortProfileID::PROFILE_DEFAULT) {
        ports.push_back(port.second.get());
      }
    }
  }
  return ports;
}

} // namespace facebook::fboss
