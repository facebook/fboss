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
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include <memory>
#include <unordered_map>

namespace facebook {
namespace fboss {

class SaiManagerTable;

class SaiHostifTrapGroup {
 public:
  SaiHostifTrapGroup(
      SaiApiTable* apiTable,
      const HostifApiParameters::Attributes& attributes,
      sai_object_id_t& switchId);
  ~SaiHostifTrapGroup();
  SaiHostifTrapGroup(const SaiHostifTrapGroup& other) = delete;
  SaiHostifTrapGroup(SaiHostifTrapGroup&& other) = delete;
  SaiHostifTrapGroup& operator=(const SaiHostifTrapGroup& other) = delete;
  SaiHostifTrapGroup& operator=(SaiHostifTrapGroup&& other) = delete;
  bool operator==(const SaiHostifTrapGroup& other) const;
  bool operator!=(const SaiHostifTrapGroup& other) const;

  const HostifApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  sai_object_id_t id_;
  HostifApiParameters::Attributes attributes_;
};

class SaiHostifTrap {
 public:
  SaiHostifTrap(
      SaiApiTable* apiTable,
      const HostifApiParameters::MemberAttributes& attributes,
      sai_object_id_t& switchId,
      std::shared_ptr<SaiHostifTrapGroup> trapGroup);
  ~SaiHostifTrap();
  SaiHostifTrap(const SaiHostifTrap& other) = delete;
  SaiHostifTrap(SaiHostifTrap&& other) = delete;
  SaiHostifTrap& operator=(const SaiHostifTrap& other) = delete;
  SaiHostifTrap& operator=(SaiHostifTrap&& other) = delete;
  bool operator==(const SaiHostifTrap& other) const;
  bool operator!=(const SaiHostifTrap& other) const;

  const HostifApiParameters::MemberAttributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  sai_object_id_t id_;
  HostifApiParameters::MemberAttributes attributes_;
  std::shared_ptr<SaiHostifTrapGroup> trapGroup_{nullptr};
};

class SaiHostifManager {
 public:
  SaiHostifManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);
  ~SaiHostifManager();
  sai_object_id_t incRefOrAddHostifTrapGroup(
      cfg::PacketRxReason trapId,
      uint32_t queueId);
  void removeHostifTrap(cfg::PacketRxReason trapId);
  std::shared_ptr<SaiHostifTrapGroup> addHostifTrapGroup(uint32_t queueId);
  void processHostifDelta(const StateDelta& delta);
  static sai_hostif_trap_type_t packetReasonToHostifTrap(
      cfg::PacketRxReason reason);
  static cfg::PacketRxReason hostifTrapToPacketReason(
      sai_hostif_trap_type_t trapType);

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  std::unordered_map<cfg::PacketRxReason, std::unique_ptr<SaiHostifTrap>>
      hostifTraps_;
  FlatRefMap<uint32_t, SaiHostifTrapGroup> hostifTrapGroups_;
  void changeHostifTrap(cfg::PacketRxReason trapId, uint32_t newQueueid);
};

} // namespace fboss
} // namespace facebook
