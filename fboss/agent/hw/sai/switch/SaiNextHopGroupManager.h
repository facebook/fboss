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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include <memory>
#include <unordered_map>

#include <boost/container/flat_set.hpp>

namespace facebook {
namespace fboss {

class SaiManagerTable;

class SaiNextHopGroupMember {
 public:
  SaiNextHopGroupMember(
      SaiApiTable* apiTable,
      const NextHopGroupApiParameters::MemberAttributes& attributes,
      const sai_object_id_t& switchID);
  ~SaiNextHopGroupMember();
  SaiNextHopGroupMember(const SaiNextHopGroupMember& other) = delete;
  SaiNextHopGroupMember(SaiNextHopGroupMember&& other) = delete;
  SaiNextHopGroupMember& operator=(const SaiNextHopGroupMember& other) = delete;
  SaiNextHopGroupMember& operator=(SaiNextHopGroupMember&& other) = delete;
  bool operator==(const SaiNextHopGroupMember& other) const;
  bool operator!=(const SaiNextHopGroupMember& other) const;

  const NextHopGroupApiParameters::MemberAttributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  NextHopGroupApiParameters::MemberAttributes attributes_;
  sai_object_id_t id_;
};

class SaiNextHopGroup {
 public:
  SaiNextHopGroup(
      SaiApiTable* apiTable,
      SaiManagerTable* managerTable,
      const NextHopGroupApiParameters::Attributes& attributes,
      const RouteNextHopEntry::NextHopSet& swNextHops);
  ~SaiNextHopGroup();
  SaiNextHopGroup(const SaiNextHopGroup& other) = delete;
  SaiNextHopGroup(SaiNextHopGroup&& other) = delete;
  SaiNextHopGroup& operator=(const SaiNextHopGroup& other) = delete;
  SaiNextHopGroup& operator=(SaiNextHopGroup&& other) = delete;
  bool operator==(const SaiNextHopGroup& other) const;
  bool operator!=(const SaiNextHopGroup& other) const;

  bool empty() const;
  std::size_t size() const;
  void addMember(sai_object_id_t nextHopId);
  void removeMember(sai_object_id_t nextHopId);
  const NextHopGroupApiParameters::Attributes attributes() const {
    return attributes_;
  }
  sai_object_id_t id() const {
    return id_;
  }

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  NextHopGroupApiParameters::Attributes attributes_;
  RouteNextHopEntry::NextHopSet swNextHops_;
  sai_object_id_t id_;
  std::unordered_map<sai_object_id_t, std::unique_ptr<SaiNextHopGroupMember>>
      members_;
  std::unordered_map<sai_object_id_t, sai_object_id_t> memberIdMap_;
};

class SaiNextHopGroupManager {
 public:
  SaiNextHopGroupManager(SaiApiTable* apiTable, SaiManagerTable* managerTable);
  std::shared_ptr<SaiNextHopGroup> incRefOrAddNextHopGroup(
      const RouteNextHopEntry::NextHopSet& swNextHops);

  void unregisterNeighborResolutionHandling(
      const RouteNextHopEntry::NextHopSet& swNextHops);
  void handleResolvedNeighbor(
      const NeighborApiParameters::EntryType& neighborEntry,
      sai_object_id_t nextHopId);
  void handleUnresolvedNeighbor(
      const NeighborApiParameters::EntryType& neighborEntry,
      sai_object_id_t nextHopId);

 private:
  SaiApiTable* apiTable_;
  SaiManagerTable* managerTable_;
  FlatRefMap<RouteNextHopEntry::NextHopSet, SaiNextHopGroup> nextHopGroups_;
  std::unordered_map<
      NeighborApiParameters::NeighborEntry,
      boost::container::flat_set<RouteNextHopEntry::NextHopSet>>
      nextHopsByNeighbor_;
};

} // namespace fboss
} // namespace facebook
