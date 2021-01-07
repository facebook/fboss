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

#include "fboss/agent/hw/sai/api/SamplePacketApi.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/types.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiSamplePacket = SaiObject<SaiSamplePacketTraits>;

class SaiSamplePacketManager {
 public:
  SaiSamplePacketManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  std::shared_ptr<SaiSamplePacket> createSamplePacket(
      uint64_t sampleRate,
      cfg::SampleDestination sampleDestination);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
