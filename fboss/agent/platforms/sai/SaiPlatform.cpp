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

namespace facebook {
namespace fboss {

HwSwitch* SaiPlatform::getHwSwitch() const  {
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

std::string SaiPlatform::getPersistentStateDir() const  {
  return std::string("/var/facebook/fboss");
}

void SaiPlatform::getProductInfo(ProductInfo& /* info */) {}

std::string SaiPlatform::getVolatileStateDir() const  {
  return std::string("/dev/shm/fboss");
}

TransceiverIdxThrift SaiPlatform::getPortMapping(PortID /* portId */) const  {
  return TransceiverIdxThrift();
}

PlatformPort* SaiPlatform::getPlatformPort(PortID /* port */) const  {
  return nullptr;
}

void SaiPlatform::initImpl() {
  saiSwitch_ = std::make_unique<SaiSwitch>();
}

} // namespace fboss
} // namespace facebook
