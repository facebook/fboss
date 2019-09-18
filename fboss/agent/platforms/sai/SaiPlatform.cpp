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
#include "fboss/agent/platforms/sai/facebook/SaiWedge400CPort.h"

DEFINE_string(
    hw_config_file,
    "hw_config",
    "File for dumping HW config on startup");

namespace {

std::unordered_map<const char*, const char*> kSaiProfileValues;

const char* saiProfileGetValue(
    sai_switch_profile_id_t /*profile_id*/,
    const char* variable) {
  auto saiProfileValItr = kSaiProfileValues.find(variable);
  return saiProfileValItr != kSaiProfileValues.end() ? saiProfileValItr->second
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
  *variable = saiProfileValItr->first;
  *value = saiProfileValItr->second;
  ++saiProfileValItr;
  return 0;
}

sai_service_method_table_t kSaiServiceMethodTable = {
    .profile_get_value = saiProfileGetValue,
    .profile_get_next_value = saiProfileGetNextValue,
};

} // namespace

namespace facebook {
namespace fboss {

HwSwitch* SaiPlatform::getHwSwitch() const {
  return saiSwitch_.get();
}

void SaiPlatform::onHwInitialized(SwSwitch* /* sw */) {}

void SaiPlatform::onInitialConfigApplied(SwSwitch* /* sw */) {}

void SaiPlatform::stop() {}

std::unique_ptr<ThriftHandler> SaiPlatform::createHandler(SwSwitch* sw) {
  return std::make_unique<ThriftHandler>(sw);
}

folly::MacAddress SaiPlatform::getLocalMac() const {
  // TODO: remove hardcoded mac
  return folly::MacAddress("42:42:42:42:42:42");
}

void SaiPlatform::getProductInfo(ProductInfo& info) {
  productInfo_->getInfo(info);
}

PlatformMode SaiPlatform::getMode() const {
  return productInfo_->getMode();
}

TransceiverIdxThrift SaiPlatform::getPortMapping(PortID /* portId */) const {
  return TransceiverIdxThrift();
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
      std::make_pair(SAI_KEY_INIT_CONFIG_FILE, getHwConfigDumpFile().c_str()));
  kSaiProfileValues.insert(std::make_pair(SAI_KEY_BOOT_TYPE, "0"));
}

void SaiPlatform::initImpl() {
  initSaiProfileValues();
  saiSwitch_ = std::make_unique<SaiSwitch>(this);
  generateHwConfigFile();
}

SaiPlatform::SaiPlatform(std::unique_ptr<PlatformProductInfo> productInfo)
    : productInfo_(std::move(productInfo)) {}

void SaiPlatform::initPorts() {
  auto& platformSettings = config()->thrift.platform;
  auto platformMode = getMode();
  for (auto& port : platformSettings.ports) {
    std::unique_ptr<SaiPlatformPort> saiPort;
    PortID portId(port.first);
    if (platformMode == PlatformMode::WEDGE400C) {
      saiPort = std::make_unique<SaiWedge400CPort>(portId, this);
    } else {
      saiPort = std::make_unique<SaiPlatformPort>(portId, this);
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

folly::Optional<std::string> SaiPlatform::getPlatformAttribute(
    cfg::PlatformAttributes platformAttribute) {
  auto& platform = config()->thrift.platform;

  auto platformIter = platform.platformSettings.find(platformAttribute);
  if (platformIter == platform.platformSettings.end()) {
    return folly::none;
  }

  return platformIter->second;
}

sai_service_method_table_t* SaiPlatform::getServiceMethodTable() const {
  return &kSaiServiceMethodTable;
}

} // namespace fboss
} // namespace facebook
