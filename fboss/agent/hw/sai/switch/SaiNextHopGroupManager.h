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
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include <memory>
#include "folly/container/F14Map.h"
#include "folly/container/F14Set.h"

#include <boost/container/flat_set.hpp>

namespace facebook::fboss {

class SaiManagerTable;
class SaiNextHopHandle;
class SaiPlatform;

using SaiNextHopGroup = SaiObject<SaiNextHopGroupTraits>;
using SaiNextHopGroupMember = SaiObject<SaiNextHopGroupMemberTraits>;

class SaiNextHopGroupMemberHandle {
 public:
  SaiNextHopGroupMemberHandle(
      std::shared_ptr<SaiNextHopHandle> nextHopHandle,
      std::shared_ptr<SaiNextHopGroupMember> member)
      : nextHopHandle_(nextHopHandle), member_(member) {}

 private:
  std::shared_ptr<SaiNextHopHandle> nextHopHandle_;
  std::shared_ptr<SaiNextHopGroupMember> member_;
};

struct SaiNextHopGroupHandle {
  std::shared_ptr<SaiNextHopGroup> nextHopGroup;
  folly::F14FastMap<
      SaiNeighborTraits::NeighborEntry,
      std::vector<std::shared_ptr<SaiNextHopGroupMemberHandle>>>
      nextHopGroupMembers;
};

class SaiNextHopGroupManager {
 public:
  SaiNextHopGroupManager(
      SaiManagerTable* managerTable,
      const SaiPlatform* platform);

  std::shared_ptr<SaiNextHopGroupHandle> incRefOrAddNextHopGroup(
      const RouteNextHopEntry::NextHopSet& swNextHops);

  void handleResolvedNeighbor(
      const SaiNeighborTraits::NeighborEntry& neighborEntry);
  void handleUnresolvedNeighbor(
      const SaiNeighborTraits::NeighborEntry& neighborEntry);

 private:
  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  // TODO(borisb): improve SaiObject/SaiStore to the point where they
  // support the next hop group use case correctly, rather than this
  // abomination of multiple levels of RefMaps :(
  FlatRefMap<RouteNextHopEntry::NextHopSet, SaiNextHopGroupHandle> handles_;
  folly::F14FastMap<
      SaiNeighborTraits::NeighborEntry,
      boost::container::flat_set<RouteNextHopEntry::NextHopSet>>
      nextHopsByNeighbor_;
};

} // namespace facebook::fboss
