/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiVlanManager::SaiVlanManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
  // default vlan
  // TODO(pshaikh): manage default vlan more graciously
  auto key = managerTable_->switchManager().getDefaultVlanAdapterKey();
  auto defaultVlan =
      saiStore_->get<SaiVlanTraits>().reloadObject(VlanSaiId(key));
  defaultVlan->release();
}

} // namespace facebook::fboss
