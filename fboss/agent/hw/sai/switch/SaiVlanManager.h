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

class SaiManagerTable;
class SaiPlatform;
class SaiVlanManager;
class SaiStore;

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
      SaiVlanManager* manager,
      SaiPortDescriptor portDesc,
      VlanID vlanId,
      SaiVlanMemberTraits::Attributes::VlanId saiVlanId,
      bool tagged)
      : Base(portDesc),
        manager_(manager),
        vlanId_(vlanId),
        saiVlanId_(saiVlanId),
        tagged_(tagged) {}

  void createObject(PublisherObjects added);

  void removeObject(size_t index, PublisherObjects removed);

  void handleLinkDown() {}

 private:
  SaiVlanManager* manager_;
  VlanID vlanId_;
  SaiVlanMemberTraits::Attributes::VlanId saiVlanId_;
  bool tagged_;
};

struct SaiVlanHandle {
  std::shared_ptr<SaiVlan> vlan;
  folly::F14FastMap<SaiPortDescriptor, std::shared_ptr<ManagedVlanMember>>
      vlanMembers;
};

class SaiVlanManager {
 public:
  SaiVlanManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);
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

  void
  createVlanMember(VlanID swVlanId, SaiPortDescriptor portDesc, bool tagged);
  void removeVlanMember(VlanID swVlanId, SaiPortDescriptor portDesc);

  std::shared_ptr<SaiVlanMember> createSaiObject(
      const typename SaiVlanMemberTraits::AdapterHostKey& key,
      const typename SaiVlanMemberTraits::CreateAttributes& attributes);

 private:
  SaiVlanHandle* getVlanHandleImpl(VlanID swVlanId) const;

  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;

  SaiVlanHandles handles_;
};

} // namespace facebook::fboss
