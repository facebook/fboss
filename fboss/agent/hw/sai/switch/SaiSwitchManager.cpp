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

std::vector<int8_t> getConnectionHandle() {
  static const char connStr[] = "/dev/testdev/socket/0.0.0.0:40000";
  return std::vector<int8_t>{std::begin(connStr), std::end(connStr)};
}

SwitchApiParameters::Attributes getSwitchAttributes(
    const SaiPlatform* platform) {
  SwitchApiParameters::Attributes::HwInfo hwInfo(getConnectionHandle());
  SwitchApiParameters::Attributes::SrcMac srcMac(platform->getLocalMac());
  SwitchApiParameters::Attributes::InitSwitch initSwitch(true);
  return {{{hwInfo}, srcMac, initSwitch}};
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
    const SaiPlatform* platform)
    : apiTable_(apiTable), managerTable_(managerTable) {
  auto defaultSwitch = std::make_unique<SaiSwitchInstance>(
      apiTable_, getSwitchAttributes(platform));
  switches_.emplace(std::make_pair(SwitchID(0), std::move(defaultSwitch)));
}

SaiSwitchInstance* SaiSwitchManager::getSwitchImpl(
    const SwitchID& switchId) const {
  auto itr = switches_.find(switchId);
  if (itr == switches_.end()) {
    return nullptr;
  }
  if (!itr->second) {
    XLOG(FATAL) << "invalid null switch for switchID: " << switchId;
  }
  return itr->second.get();
}

const SaiSwitchInstance* SaiSwitchManager::getSwitch(
    const SwitchID& switchId) const {
  return getSwitchImpl(switchId);
}

SaiSwitchInstance* SaiSwitchManager::getSwitch(const SwitchID& switchId) {
  return getSwitchImpl(switchId);
}

sai_object_id_t SaiSwitchManager::getSwitchSaiId(
    const SwitchID& switchId) const {
  auto switchInstance = getSwitch(switchId);
  if (!switchInstance) {
    throw FbossError("Attempted to query non-existing switch: ", switchId);
  }
  return switchInstance->id();
}

} // namespace fboss
} // namespace facebook
