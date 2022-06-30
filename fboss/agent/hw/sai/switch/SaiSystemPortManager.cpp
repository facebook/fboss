/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

#include <fmt/ranges.h>

using namespace std::chrono;

namespace facebook::fboss {
SaiSystemPortManager::SaiSystemPortManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

SystemPortSaiId SaiSystemPortManager::addSystemPort(
    const std::shared_ptr<SystemPort>& /*swSystemPort*/) {
  // TODO
  SystemPortSaiId portSaiId;
  return portSaiId;
}

void SaiSystemPortManager::removeSystemPort(
    const std::shared_ptr<SystemPort>& swSystemPort) {
  auto swId = swSystemPort->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Attempted to remove non-existent system port: ", swId);
  }
  handles_.erase(itr);
  XLOG(INFO) << "removed system port " << swSystemPort->getID();
}

// private const getter for use by const and non-const getters
SaiSystemPortHandle* SaiSystemPortManager::getSystemPortHandleImpl(
    SystemPortID swId) const {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiSystemPortHandle for " << swId;
  }
  return itr->second.get();
}

} // namespace facebook::fboss
