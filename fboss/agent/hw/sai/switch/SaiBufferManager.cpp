/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiBufferManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiBufferManager::SaiBufferManager(
    SaiManagerTable* managerTable,
    const SaiPlatform* platform)
    : managerTable_(managerTable), platform_(platform) {}

void SaiBufferManager::setupEgressBufferPool() {
  egressBufferPoolHandle_ = std::make_unique<SaiBufferPoolHandle>();
  auto& store = SaiStore::getInstance()->get<SaiBufferPoolTraits>();
  auto mmuSize = platform_->getAsic()->getMMUSizeBytes();
  SaiBufferPoolTraits::CreateAttributes c{
      SAI_BUFFER_POOL_TYPE_EGRESS,
      mmuSize,
      SAI_BUFFER_POOL_THRESHOLD_MODE_DYNAMIC};
  store.setObject(SAI_BUFFER_POOL_TYPE_EGRESS, c);
}
} // namespace facebook::fboss
