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
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class PortQueue;

using SaiBufferPool = SaiObjectWithCounters<SaiBufferPoolTraits>;
using SaiBufferProfile = SaiObject<SaiBufferProfileTraits>;

struct SaiBufferPoolHandle {
  std::shared_ptr<SaiBufferPool> bufferPool;
};

class SaiBufferManager {
 public:
  SaiBufferManager(SaiManagerTable* managerTable, const SaiPlatform* platform);

  std::shared_ptr<SaiBufferProfile> getOrCreateProfile(const PortQueue& queue);

  void setupEgressBufferPool();
  void updateStats();
  uint64_t getDeviceWatermarkBytes() const {
    return deviceWatermarkBytes_;
  }

 private:
  void publishDeviceWatermark(uint64_t peakBytes) const;
  SaiBufferProfileTraits::CreateAttributes profileCreateAttrs(
      const PortQueue& queue) const;

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  std::unique_ptr<SaiBufferPoolHandle> egressBufferPoolHandle_;
  UnorderedRefMap<SaiBufferProfileTraits::AdapterHostKey, SaiBufferProfile>
      bufferProfiles_;
  uint64_t deviceWatermarkBytes_{0};
};

} // namespace facebook::fboss
