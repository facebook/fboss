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

#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/api/VlanApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber-defs.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"

#include "folly/container/F14Map.h"

#include <memory>
#include <mutex>
#include <optional>

namespace facebook::fboss {

class ConcurrentIndices;
class SaiManagerTable;
class SaiPlatform;

using SaiVlan = SaiObject<SaiVlanTraits>;
using SaiVlanMember = SaiObject<SaiVlanMemberTraits>;

class ManagedVlanMember : public SaiObjectEventAggregateSubscriber<
                              ManagedVlanMember,
                              SaiVlanMemberTraits,
                              SaiBridgePortTraits> {
 public:
  using Base = SaiObjectEventAggregateSubscriber<
      ManagedVlanMember,
      SaiVlanMemberTraits,
      SaiBridgePortTraits>;
  using BridgePortWeakPtr = std::weak_ptr<const SaiObject<SaiBridgePortTraits>>;
  using PublisherObjects = std::tuple<BridgePortWeakPtr>;

  ManagedVlanMember(
      PortID portId,
      VlanID vlanId,
      SaiVlanMemberTraits::Attributes::VlanId saiVlanId)
      : Base(portId), vlanId_(vlanId), saiVlanId_(saiVlanId) {}

  void createObject(PublisherObjects added);

  void removeObject(size_t index, PublisherObjects removed);

 private:
  VlanID vlanId_;
  SaiVlanMemberTraits::Attributes::VlanId saiVlanId_;
};

struct SaiVlanHandle {
  std::shared_ptr<SaiVlan> vlan;
  folly::F14FastMap<PortID, std::shared_ptr<ManagedVlanMember>> vlanMembers;
};

class SaiVlanManager {
 public:
  SaiVlanManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform,
      ConcurrentIndices* concurrentIndices);
  using SaiVlanHandles =
      folly::F14FastMap<VlanID, std::unique_ptr<SaiVlanHandle>>;

  VlanSaiId addVlan(const std::shared_ptr<Vlan>& swVlan);
  void removeVlan(const std::shared_ptr<Vlan>& swVlan);
  void changeVlan(
      const std::shared_ptr<Vlan>& swVlanOld,
      const std::shared_ptr<Vlan>& swVlanNew);

  const SaiVlanHandle* getVlanHandle(VlanID swVlanId) const;
  SaiVlanHandle* getVlanHandle(VlanID swVlanId);

  const SaiVlanHandles& getVlanHandles() const {
    return handles_;
  }

 private:
  void createVlanMember(VlanID swVlanId, PortID swPortId);
  SaiVlanHandle* getVlanHandleImpl(VlanID swVlanId) const;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;

  SaiVlanHandles handles_;
};

} // namespace facebook::fboss
