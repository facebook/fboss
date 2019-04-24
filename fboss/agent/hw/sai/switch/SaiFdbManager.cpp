/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiFdbManager.h"
#include "fboss/agent/hw/sai/switch/SaiBridgeManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

namespace facebook {
namespace fboss {

SaiFdbEntry::SaiFdbEntry(
    SaiApiTable* apiTable,
    const FdbApiParameters::EntryType& entry,
    const FdbApiParameters::Attributes& attributes)
    : apiTable_(apiTable),
      entry_(entry),
      attributes_(attributes) {
  auto& fdbApi = apiTable_->fdbApi();
  fdbApi.create(entry_, attributes.attrs());
}

SaiFdbEntry::~SaiFdbEntry() {
  auto& fdbApi = apiTable_->fdbApi();
  fdbApi.remove(entry_);
}

bool SaiFdbEntry::operator==(const SaiFdbEntry& other) const {
  return attributes_ == other.attributes_;
}

bool SaiFdbEntry::operator!=(const SaiFdbEntry& other) const {
  return !(*this == other);
}

SaiFdbManager::SaiFdbManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable)
    : apiTable_(apiTable), managerTable_(managerTable) {}

std::unique_ptr<SaiFdbEntry> SaiFdbManager::addFdbEntry(
  const InterfaceID& intfId,
  const folly::MacAddress& mac,
  const PortDescriptor& portDesc) {
  XLOG(INFO) << "addFdb " << mac;
  SaiRouterInterface* routerInterface = managerTable_->routerInterfaceManager()
        .getRouterInterface(intfId);
  if (!routerInterface) {
    throw FbossError(
      "Attempted to add non-existent interface to Fdb: ", intfId);
  }
  auto vlanId = routerInterface->attributes().vlanId;
  // TODO(srikrishnagopu): Can it be an AGG Port ?
  auto portId = portDesc.phyPortID();
  auto port = managerTable_->portManager().getPort(portId);
  if (!port) {
    throw FbossError(
      "Attempted to add non-existent port to Fdb: ", portId);
  }
  auto switchId = managerTable_->switchManager().getSwitchSaiId(SwitchID(0));
  auto bridgePortId = port->getBridgePort()->id();
  FdbApiParameters::EntryType entry{switchId, vlanId, mac};
  FdbApiParameters::Attributes attributes{{
    SAI_FDB_ENTRY_TYPE_STATIC, bridgePortId}};
  return std::make_unique<SaiFdbEntry>(apiTable_, entry, attributes);
}

} // namespace fboss
} // namespace facebook
