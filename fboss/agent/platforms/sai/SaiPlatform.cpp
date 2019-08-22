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

namespace facebook {
namespace fboss {

HwSwitch* SaiPlatform::getHwSwitch() const {
  return saiSwitch_.get();
}

void SaiPlatform::onHwInitialized(SwSwitch* /* sw */) {}

void SaiPlatform::onInitialConfigApplied(SwSwitch* /* sw */) {}

std::unique_ptr<ThriftHandler> SaiPlatform::createHandler(SwSwitch* sw) {
  return std::make_unique<ThriftHandler>(sw);
}

folly::MacAddress SaiPlatform::getLocalMac() const {
  return folly::MacAddress("42:42:42:42:42:42");
}

std::string SaiPlatform::getPersistentStateDir() const {
  return std::string("/var/facebook/fboss");
}

void SaiPlatform::getProductInfo(ProductInfo& info) {
  productInfo_->getInfo(info);
}

PlatformMode SaiPlatform::getMode() const {
  return productInfo_->getMode();
}

std::string SaiPlatform::getVolatileStateDir() const {
  return std::string("/dev/shm/fboss");
}

TransceiverIdxThrift SaiPlatform::getPortMapping(PortID /* portId */) const {
  return TransceiverIdxThrift();
}

void SaiPlatform::initImpl() {
  saiSwitch_ = std::make_unique<SaiSwitch>(this);
}

std::vector<int8_t> SaiPlatform::getConnectionHandle() const {
  static const std::array<int8_t, 34> connStr{
      "/dev/testdev/socket/0.0.0.0:40000"};
  return std::vector<int8_t>{std::begin(connStr), std::end(connStr)};
}

SaiPlatform::SaiPlatform(std::unique_ptr<PlatformProductInfo> productInfo)
    : productInfo_(std::move(productInfo)) {}

void SaiPlatform::initPorts() {
  auto platformSettings = config()->thrift.get_platform();
  if (!platformSettings) {
    throw FbossError("platform config is empty");
  }
  auto platformMode = getMode();
  for (auto& port : platformSettings->ports) {
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

} // namespace fboss
} // namespace facebook
