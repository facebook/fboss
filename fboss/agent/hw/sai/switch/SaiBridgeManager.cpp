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

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

SaiBridgePort::SaiBridgePort(
    SaiApiTable* apiTable,
    const BridgeApiParameters::MemberAttributes& attributes)
    : apiTable_(apiTable), attributes_(attributes) {
  auto& bridgeApi = apiTable_->bridgeApi();
  id_ = bridgeApi.createMember2(attributes_.attrs(), 0);
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
  BridgeApiParameters::MemberAttributes attributes{
      {SAI_BRIDGE_PORT_TYPE_PORT, portId}};
  return std::make_unique<SaiBridgePort>(apiTable_, attributes);
}

SaiBridgeManager::SaiBridgeManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable)
    : apiTable_(apiTable), managerTable_(managerTable) {}

} // namespace fboss
} // namespace facebook
