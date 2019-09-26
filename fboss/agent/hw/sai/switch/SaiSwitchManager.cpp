/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

// (TODO: srikrishnagopu) Move this to SaiPlatform ?
namespace {
using namespace facebook::fboss;

SaiSwitchTraits::CreateAttributes getSwitchAttributes(SaiPlatform* platform) {
  std::vector<int8_t> connectionHandle;
  auto connStr = platform->getPlatformAttribute(
      cfg::PlatformAttributes::CONNECTION_HANDLE);
  if (connStr.hasValue()) {
    std::copy(
        connStr->c_str(),
        connStr->c_str() + connStr->size(),
        std::back_inserter(connectionHandle));
  }
  SaiSwitchTraits::Attributes::InitSwitch initSwitch(true);
  SaiSwitchTraits::Attributes::HwInfo hwInfo(connectionHandle);
  SaiSwitchTraits::Attributes::SrcMac srcMac(platform->getLocalMac());
  return {initSwitch, hwInfo, srcMac};
}
} // namespace

namespace facebook {
namespace fboss {

SaiSwitchInstance::SaiSwitchInstance(
    const SaiSwitchTraits::CreateAttributes& attributes)
    : attributes_(attributes) {
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  id_ = switchApi.create<SaiSwitchTraits>(
      attributes_, 0 /* fake switch id; ignored */);
}

SaiSwitchInstance::~SaiSwitchInstance() {
  auto& switchApi = SaiApiTable::getInstance()->switchApi();
  switchApi.remove(id());
}

bool SaiSwitchInstance::operator==(const SaiSwitchInstance& other) const {
  return attributes_ == other.attributes();
}
bool SaiSwitchInstance::operator!=(const SaiSwitchInstance& other) const {
  return !(*this == other);
}

SaiSwitchManager::SaiSwitchManager(
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {
  switch_ = std::make_unique<SaiSwitchInstance>(getSwitchAttributes(platform));
}

SaiSwitchInstance* SaiSwitchManager::getSwitchImpl() const {
  if (!switch_) {
    XLOG(FATAL) << "invalid null switch";
  }
  return switch_.get();
}

const SaiSwitchInstance* SaiSwitchManager::getSwitch() const {
  return getSwitchImpl();
}

SaiSwitchInstance* SaiSwitchManager::getSwitch() {
  return getSwitchImpl();
}

SwitchSaiId SaiSwitchManager::getSwitchSaiId() const {
  auto switchInstance = getSwitch();
  if (!switchInstance) {
    throw FbossError("failed to get switch id: switch not initialized");
  }
  return switchInstance->id();
}

} // namespace fboss
} // namespace facebook
