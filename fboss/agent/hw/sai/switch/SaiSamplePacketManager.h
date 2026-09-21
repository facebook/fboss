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

enum class SamplePacketDirection { INGRESS = 1, EGRESS = 2 };
enum class SamplePacketAction { START = 1, STOP = 2 };

class SaiManagerTable;
class SaiPlatform;
class SaiStore;

using SaiSamplePacket = SaiObject<SaiSamplePacketTraits>;

class SaiSamplePacketManager {
 public:
  SaiSamplePacketManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  std::shared_ptr<SaiSamplePacket> getOrCreateSamplePacket(
      uint64_t sampleRate,
      cfg::SampleDestination sampleDestination);

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
};

} // namespace facebook::fboss
