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

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/VlanApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include "folly/container/F14Map.h"

#include <memory>
#include <optional>

namespace facebook {
namespace fboss {

class SaiManagerTable;
class SaiPlatform;

using SaiVlan = SaiObject<SaiVlanTraits>;
using SaiVlanMember = SaiObject<SaiVlanMemberTraits>;

struct SaiVlanHandle {
  std::shared_ptr<SaiVlan> vlan;
  folly::F14FastMap<PortID, std::shared_ptr<SaiVlanMember>> vlanMembers;
};

class SaiVlanManager {
 public:
  SaiVlanManager(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
  VlanSaiId addVlan(const std::shared_ptr<Vlan>& swVlan);
  void removeVlan(const VlanID& swVlanId);
  void changeVlan(
      const std::shared_ptr<Vlan>& swVlanOld,
      const std::shared_ptr<Vlan>& swVlanNew);
  void processVlanDelta(const VlanMapDelta& delta);

  const SaiVlanHandle* getVlanHandle(VlanID swVlanId) const;
  SaiVlanHandle* getVlanHandle(VlanID swVlanId);

  // TODO(borisb): remove after D15750266
  VlanID getVlanIdByPortId(PortID portId) const;

 private:
  void createVlanMember(VlanID swVlanId, PortID swPortId);
  SaiVlanHandle* getVlanHandleImpl(VlanID swVlanId) const;
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;

  folly::F14FastMap<VlanID, std::unique_ptr<SaiVlanHandle>> handles_;
  // TODO(borisb): remove after D15750266
  folly::F14FastMap<PortID, VlanID> vlanIdsByPortId_;
};

} // namespace fboss
} // namespace facebook
