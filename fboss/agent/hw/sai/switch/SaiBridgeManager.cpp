/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

SaiBridge::SaiBridge(
    SaiApiTable* apiTable,
    const BridgeApiParameters::Attributes& attributes,
    const sai_object_id_t& switchId)
    : apiTable_(apiTable), attributes_(attributes) {
  try {
    id_ = apiTable_->switchApi().getAttribute(
        SwitchApiParameters::Attributes::Default1QBridgeId(), switchId);
  } catch (const facebook::fboss::SaiApiError& ex) {
    // Create a default 1Q Bridge if the SAI sdk adapter
    // did not return a default 1Q bridge and throws an exception.
    auto& bridgeApi = apiTable_->bridgeApi();
    id_ = bridgeApi.create(attributes_.attrs(), switchId);
  }
}

SaiBridge::~SaiBridge() {
  auto& bridgeApi = apiTable_->bridgeApi();
  bridgeApi.remove(id());
}

SaiBridgePort::SaiBridgePort(
    SaiApiTable* apiTable,
    const BridgeApiParameters::MemberAttributes& attributes,
    const sai_object_id_t& switchId)
    : apiTable_(apiTable), attributes_(attributes) {
  auto& bridgeApi = apiTable_->bridgeApi();
  id_ = bridgeApi.createMember(attributes_.attrs(), switchId);
}

SaiBridgePort::~SaiBridgePort() {
  auto& bridgeApi = apiTable_->bridgeApi();
  bridgeApi.removeMember(id_);
}

bool SaiBridgePort::operator==(const SaiBridgePort& other) const {
  return attributes_ == other.attributes();
}
bool SaiBridgePort::operator!=(const SaiBridgePort& other) const {
  return !(*this == other);
}

std::unique_ptr<SaiBridgePort> SaiBridgeManager::addBridgePort(
    sai_object_id_t portId) {
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  BridgeApiParameters::MemberAttributes attributes{
      {SAI_BRIDGE_PORT_TYPE_PORT, portId}};
  return std::make_unique<SaiBridgePort>(apiTable_, attributes, switchId);
}

SaiBridgeManager::SaiBridgeManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : apiTable_(apiTable), managerTable_(managerTable), platform_(platform) {
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  BridgeApiParameters::Attributes attributes{{SAI_BRIDGE_TYPE_1Q}};
  auto defaultBridge =
      std::make_unique<SaiBridge>(apiTable_, attributes, switchId);
  bridges_.emplace(std::make_pair(BridgeID(0), std::move(defaultBridge)));
}
} // namespace fboss
} // namespace facebook
