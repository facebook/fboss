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

SwitchApiParameters::Attributes getSwitchAttributes(SaiPlatform* platform) {
  SwitchApiParameters::Attributes::InitSwitch initSwitch(true);
  auto connStr = platform->getPlatformAttribute(
      cfg::PlatformAttributes::CONNECTION_HANDLE);
  auto connectionHandle = std::vector<int8_t>{std::begin(connStr.value()),
                                              std::end(connStr.value())};
  SwitchApiParameters::Attributes::HwInfo hwInfo(connectionHandle);
  SwitchApiParameters::Attributes::SrcMac srcMac(platform->getLocalMac());
  return {{initSwitch, hwInfo, srcMac}};
}
} // namespace

namespace facebook {
namespace fboss {

SaiSwitchInstance::SaiSwitchInstance(
    SaiApiTable* apiTable,
    const SwitchApiParameters::Attributes& attributes)
    : apiTable_(apiTable), attributes_(attributes) {
  auto& switchApi = apiTable_->switchApi();
  id_ = switchApi.create(attributes_.attrs());
}

SaiSwitchInstance::~SaiSwitchInstance() {
  auto& switchApi = apiTable_->switchApi();
  switchApi.remove(id());
}

bool SaiSwitchInstance::operator==(const SaiSwitchInstance& other) const {
  return attributes_ == other.attributes();
}
bool SaiSwitchInstance::operator!=(const SaiSwitchInstance& other) const {
  return !(*this == other);
}

SaiSwitchManager::SaiSwitchManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : apiTable_(apiTable), managerTable_(managerTable), platform_(platform) {
  switch_ = std::make_unique<SaiSwitchInstance>(
      apiTable_, getSwitchAttributes(platform));
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

sai_object_id_t SaiSwitchManager::getSwitchSaiId() const {
  auto switchInstance = getSwitch();
  if (!switchInstance) {
    throw FbossError("failed to get switch id: switch not initialized");
  }
  return switchInstance->id();
}

} // namespace fboss
} // namespace facebook
