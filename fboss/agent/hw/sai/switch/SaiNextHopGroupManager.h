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

#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"

#include "fboss/agent/hw/sai/api/ArsApi.h"
#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/switch/SaiArsManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopManager.h"
#include "fboss/agent/state/FlowletSwitchingConfig.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include <memory>
#include "folly/container/F14Map.h"
#include "folly/container/F14Set.h"

#include <boost/container/flat_set.hpp>

#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber-defs.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"

namespace facebook::fboss {

class SaiArsManager;
class SaiManagerTable;
class SaiPlatform;
class SaiNextHopGroupManager;
class SaiStore;
struct SaiNextHopGroupHandle;

using SaiNextHopGroup = SaiObject<SaiNextHopGroupTraits>;
using SaiNextHopGroupMember = SaiObject<SaiNextHopGroupMemberTraits>;
using SaiNextHop = ConditionSaiObjectType<SaiNextHopTraits>::type;
using SaiNextHopGroupMemberInfo = std::pair<
    SaiNextHopGroupMemberTraits::AdapterHostKey,
    SaiNextHopGroupMemberTraits::Attributes::Weight>;
using SaiNextHopGroupKey =
    std::pair<RouteNextHopEntry::NextHopSet, std::optional<cfg::SwitchingMode>>;

template <typename T>
class ManagedNextHop;

template <typename NextHopTraits>
class ManagedSaiNextHopGroupMember
    : public SaiObjectEventAggregateSubscriber<
          ManagedSaiNextHopGroupMember<NextHopTraits>,
          SaiNextHopGroupMemberTraits,
          NextHopTraits> {
 public:
  using Base = SaiObjectEventAggregateSubscriber<
      ManagedSaiNextHopGroupMember<NextHopTraits>,
      SaiNextHopGroupMemberTraits,
      NextHopTraits>;

  using NextHopWeakPtr = std::weak_ptr<const SaiObject<NextHopTraits>>;
  using PublisherObjects = std::tuple<NextHopWeakPtr>;
  using NextHopWeight =
      typename SaiNextHopGroupMemberTraits::Attributes::Weight;
  ManagedSaiNextHopGroupMember(
      SaiNextHopGroupManager* manager,
      SaiNextHopGroupHandle* nhgroup,
      std::shared_ptr<ManagedNextHop<NextHopTraits>> managedNextHop,
      SaiNextHopGroupTraits::AdapterKey nexthopGroupId,
      NextHopWeight weight,
      bool fixedWidthMode)
      : Base(managedNextHop->adapterHostKey()),
        manager_(manager),
        nhgroup_(nhgroup),
        managedNextHop_(managedNextHop),
        nexthopGroupId_(nexthopGroupId),
        weight_(weight),
        fixedWidthMode_(fixedWidthMode) {}

  ~ManagedSaiNextHopGroupMember() {
    this->resetObject();
  }

  std::pair<
      std::optional<SaiNextHopGroupMemberTraits::AdapterHostKey>,
      std::optional<SaiNextHopGroupMemberTraits::CreateAttributes>>
  getAdapterHostKeyAndCreateAttributes();

  void setObjectOwnership(
      const std::shared_ptr<SaiNextHopGroupMember>& object) {
    this->setObject(object);
  }

  std::shared_ptr<SaiObject<SaiNextHopGroupMemberTraits>>
  getNhopGroupMemberObject() {
    return this->getObject();
  }

  void createObject(PublisherObjects added);

  void removeObject(size_t index, PublisherObjects removed);

  void handleLinkDown() {}

  std::string toString() const;

 private:
  SaiNextHopGroupManager* manager_;
  SaiNextHopGroupHandle* nhgroup_;
  std::shared_ptr<ManagedNextHop<NextHopTraits>> managedNextHop_;
  SaiNextHopGroupTraits::AdapterKey nexthopGroupId_;
  NextHopWeight weight_;
  bool fixedWidthMode_;
  std::optional<SaiNextHopGroupMemberTraits::AdapterHostKey> adapterHostKey_;
  std::optional<SaiNextHopGroupMemberTraits::CreateAttributes>
      createAttributes_;
};

class NextHopGroupMember {
 public:
  using ManagedIpNextHopGroupMember =
      ManagedSaiNextHopGroupMember<SaiIpNextHopTraits>;
  using ManagedMplsNextHopGroupMember =
      ManagedSaiNextHopGroupMember<SaiMplsNextHopTraits>;

  NextHopGroupMember(
      SaiNextHopGroupManager* manager,
      SaiNextHopGroupHandle* nhgroup,
      SaiNextHopGroupTraits::AdapterKey nexthopGroupId,
      ManagedSaiNextHop managedSaiNextHop,
      NextHopWeight nextHopWeight,
      bool fixedWidthMode);

  bool isProgrammed() const {
    return std::visit(
        [](auto arg) { return arg && arg->isAlive(); },
        managedNextHopGroupMember_);
  }

  std::string toString() {
    return std::visit(
        [](auto arg) {
          if (!arg) {
            return std::string("");
          }
          return arg->toString();
        },
        managedNextHopGroupMember_);
  }

  std::pair<
      std::optional<SaiNextHopGroupMemberTraits::AdapterHostKey>,
      std::optional<SaiNextHopGroupMemberTraits::CreateAttributes>>
  getAdapterHostKeyAndCreateAttributes() {
    return std::visit(
        [](auto arg) {
          std::pair<
              std::optional<SaiNextHopGroupMemberTraits::AdapterHostKey>,
              std::optional<SaiNextHopGroupMemberTraits::CreateAttributes>>
              ret;
          if (arg) {
            ret = arg->getAdapterHostKeyAndCreateAttributes();
          }
          return ret;
        },
        managedNextHopGroupMember_);
  }

  void setObject(const std::shared_ptr<SaiNextHopGroupMember>& object) {
    return std::visit(
        [&](auto arg) {
          CHECK(arg);
          return arg->setObjectOwnership(object);
        },
        managedNextHopGroupMember_);
  }

  std::shared_ptr<SaiObject<SaiNextHopGroupMemberTraits>> getObject() {
    return std::visit(
        [&](auto arg) {
          std::shared_ptr<SaiObject<SaiNextHopGroupMemberTraits>> object;
          if (arg) {
            object = arg->getNhopGroupMemberObject();
          }
          return object;
        },
        managedNextHopGroupMember_);
  }

 private:
  std::variant<
      std::shared_ptr<ManagedIpNextHopGroupMember>,
      std::shared_ptr<ManagedMplsNextHopGroupMember>>
      managedNextHopGroupMember_;
};

struct SaiNextHopGroupHandle {
  std::shared_ptr<SaiNextHopGroup> nextHopGroup;
  std::vector<std::shared_ptr<NextHopGroupMember>> members_;
  bool fixedWidthMode{false};
  bool bulkCreate{false};
  std::set<SaiNextHopGroupMemberInfo> fixedWidthNextHopGroupMembers_;
  uint32_t maxVariableWidthEcmpSize;
  std::optional<cfg::SwitchingMode> desiredArsMode_;
  SaiStore* saiStore_;
  const SaiPlatform* platform_;
  sai_object_id_t adapterKey() const {
    if (!nextHopGroup) {
      return SAI_NULL_OBJECT_ID;
    }
    return nextHopGroup->adapterKey();
  }
  size_t nextHopGroupSize() const;
  void memberAdded(
      SaiNextHopGroupMemberInfo memberInfo,
      bool updateHardware = true);
  void memberRemoved(
      SaiNextHopGroupMemberInfo memberInfo,
      bool updateHardware = true);
  void bulkProgramMembers(
      SaiNextHopGroupMemberInfo modifiedMemberInfo,
      bool added,
      bool updateHardware);
  ~SaiNextHopGroupHandle();
};

class SaiNextHopGroupManager {
 public:
  SaiNextHopGroupManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  std::shared_ptr<SaiNextHopGroupHandle> incRefOrAddNextHopGroup(
      const SaiNextHopGroupKey& key);

  std::shared_ptr<SaiNextHopGroupMember> createSaiObject(
      const typename SaiNextHopGroupMemberTraits::AdapterHostKey& key,
      const typename SaiNextHopGroupMemberTraits::CreateAttributes& attributes);

  std::shared_ptr<SaiNextHopGroupMember> getSaiObject(
      const typename SaiNextHopGroupMemberTraits::AdapterHostKey& key);

  std::shared_ptr<SaiNextHopGroupMember> getSaiObjectFromWBCache(
      const typename SaiNextHopGroupMemberTraits::AdapterHostKey& key);

  std::string listManagedObjects() const;

  void updateArsModeAll(
      const std::shared_ptr<FlowletSwitchingConfig>& newFlowletConfig);

  void setPrimaryArsSwitchingMode(
      std::optional<cfg::SwitchingMode> switchingMode);

  cfg::SwitchingMode getNextHopGroupSwitchingMode(
      const RouteNextHopEntry::NextHopSet& swNextHops);

 private:
  bool isFixedWidthNextHopGroup(
      const RouteNextHopEntry::NextHopSet& swNextHops) const;
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  // TODO(borisb): improve SaiObject/SaiStore to the point where they
  // support the next hop group use case correctly, rather than this
  // abomination of multiple levels of RefMaps :(
  FlatRefMap<SaiNextHopGroupKey, SaiNextHopGroupHandle> handles_;
  UnorderedRefMap<
      std::pair<typename SaiNextHopGroupTraits::AdapterKey, ResolvedNextHop>,
      NextHopGroupMember>
      nextHopGroupMembers_;
  std::optional<cfg::SwitchingMode> primaryArsMode_;
};

} // namespace facebook::fboss
