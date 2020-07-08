/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/sai/api/BufferApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiBufferPool = SaiObject<SaiBufferPoolTraits>;

struct SaiBufferPoolHandle {
  std::shared_ptr<SaiBufferPool> bufferPool;
};

class SaiBufferManager {
 public:
  SaiBufferManager(SaiManagerTable* managerTable, const SaiPlatform* platform);
  void setupEgressBufferPool();

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::unique_ptr<SaiBufferPoolHandle> egressBufferPoolHandle_;
};

} // namespace facebook::fboss
