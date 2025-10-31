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

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/hw/sai/api/SwitchApi.h"

namespace facebook::fboss {

void SaiBufferManager::loadCpuPortEgressBufferPool() {
  const auto& switchApi = SaiApiTable::getInstance()->switchApi();
  auto id = managerTable_->switchManager().getSwitchSaiId();
  auto cpuBufferPool = static_cast<BufferPoolSaiId>(switchApi.getAttribute(
      id, SaiSwitchTraits::Attributes::DefaultCpuEgressBufferPool{}));

  auto egressCpuPortBufferPoolHandle = std::make_unique<SaiBufferPoolHandle>();
  auto& store = saiStore_->get<SaiBufferPoolTraits>();
  egressCpuPortBufferPoolHandle->bufferPool =
      store.loadObjectOwnedByAdapter(cpuBufferPool);

  egressCpuPortBufferPoolHandle_ = std::move(egressCpuPortBufferPoolHandle);
}
} // namespace facebook::fboss
