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

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
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

class SaiManagerTable;
class SaiPlatform;

using SaiNextHopGroup = SaiObject<SaiNextHopGroupTraits>;
using SaiNextHopGroupMember = SaiObject<SaiNextHopGroupMemberTraits>;
using SaiNextHop = ConditionSaiObjectType<SaiNextHopTraits>::type;

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
      SaiNextHopGroupTraits::AdapterKey nexthopGroupId,
      NextHopWeight weight,
      typename PublisherKey<NextHopTraits>::type attrs)
      : Base(attrs), nexthopGroupId_(nexthopGroupId), weight_(weight) {}

  void createObject(PublisherObjects added) {
    CHECK(this->allPublishedObjectsAlive()) << "next hops are not ready";

    auto nexthopId = std::get<NextHopWeakPtr>(added).lock()->adapterKey();

    SaiNextHopGroupMemberTraits::AdapterHostKey adapterHostKey{nexthopGroupId_,
                                                               nexthopId};
    SaiNextHopGroupMemberTraits::CreateAttributes createAttributes{
        nexthopGroupId_, nexthopId, weight_};

    this->setObject(adapterHostKey, createAttributes);
  }

  void removeObject(size_t /*index*/, PublisherObjects /*removed*/) {
    /* remove nexthop group member if next hop is removed */
    this->resetObject();
  }

 private:
  SaiNextHopGroupTraits::AdapterKey nexthopGroupId_;
  NextHopWeight weight_;
};

class ManagedNextHopGroupMember {
 public:
  using ManagedIpNextHopGroupMember =
      ManagedSaiNextHopGroupMember<SaiIpNextHopTraits>;
  using ManagedMplsNextHopGroupMember =
      ManagedSaiNextHopGroupMember<SaiMplsNextHopTraits>;

  ManagedNextHopGroupMember(
      SaiManagerTable* managerTable,
      SaiNextHopGroupTraits::AdapterKey nexthopGroupId,
      const ResolvedNextHop& nexthop);

  bool isAlive() const {
    return std::visit(
        [](auto arg) { return arg && arg->isAlive(); },
        managedNextHopGroupMember_);
  }

 private:
  std::variant<
      std::shared_ptr<ManagedNextHop<SaiIpNextHopTraits>>,
      std::shared_ptr<ManagedNextHop<SaiMplsNextHopTraits>>>
      managedNextHop_;

  std::variant<
      std::shared_ptr<ManagedIpNextHopGroupMember>,
      std::shared_ptr<ManagedMplsNextHopGroupMember>>
      managedNextHopGroupMember_;
};

struct SaiNextHopGroupHandle {
  std::shared_ptr<SaiNextHopGroup> nextHopGroup;
  std::vector<std::shared_ptr<ManagedNextHopGroupMember>> members_;
  sai_object_id_t adapterKey() const {
    if (!nextHopGroup) {
      return SAI_NULL_OBJECT_ID;
    }
    return nextHopGroup->adapterKey();
  }
};

class SaiNextHopGroupManager {
 public:
  SaiNextHopGroupManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  std::shared_ptr<SaiNextHopGroupHandle> incRefOrAddNextHopGroup(
      const RouteNextHopEntry::NextHopSet& swNextHops);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  // TODO(borisb): improve SaiObject/SaiStore to the point where they
  // support the next hop group use case correctly, rather than this
  // abomination of multiple levels of RefMaps :(
  FlatRefMap<RouteNextHopEntry::NextHopSet, SaiNextHopGroupHandle> handles_;
  FlatRefMap<
      std::pair<typename SaiNextHopGroupTraits::AdapterKey, ResolvedNextHop>,
      ManagedNextHopGroupMember>
      managedNextHopGroupMembers_;
};

} // namespace facebook::fboss
