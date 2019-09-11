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

#include "fboss/agent/hw/sai/api/HostifApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include <memory>
#include "folly/container/F14Map.h"

namespace facebook {
namespace fboss {

class SaiManagerTable;

using SaiHostifTrapGroup = SaiObject<SaiHostifTrapGroupTraits>;
using SaiHostifTrap = SaiObject<SaiHostifTrapTraits>;

struct SaiHostifTrapHandle {
  std::shared_ptr<SaiHostifTrap> trap;
  std::shared_ptr<SaiHostifTrapGroup> trapGroup;
};

class SaiHostifManager {
 public:
  SaiHostifManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);

  HostifTrapSaiId addHostifTrap(cfg::PacketRxReason trapId, uint32_t queueId);
  void removeHostifTrap(cfg::PacketRxReason trapId);
  void changeHostifTrap(cfg::PacketRxReason trapId, uint32_t queueId);

  void processControlPlaneDelta(const StateDelta& delta);

  static sai_hostif_trap_type_t packetReasonToHostifTrap(
      cfg::PacketRxReason reason);
  static cfg::PacketRxReason hostifTrapToPacketReason(
      sai_hostif_trap_type_t trapType);
  static SaiHostifTrapTraits::CreateAttributes makeHostifTrapAttributes(
      cfg::PacketRxReason trapId,
      HostifTrapGroupSaiId trapGroupId);

 private:
  std::shared_ptr<SaiHostifTrapGroup> ensureHostifTrapGroup(uint32_t queueId);

  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  folly::F14FastMap<cfg::PacketRxReason, std::unique_ptr<SaiHostifTrapHandle>>
      handles_;
};

} // namespace fboss
} // namespace facebook
