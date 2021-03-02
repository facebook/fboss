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

#include <folly/IPAddress.h>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class SwitchState;

/*
 * Wrapper class to handle route updates and programming across both
 * stand alone RIB and legacy setups
 */
class RouteUpdateWrapper {
  struct AddDelRoutes {
    std::vector<UnicastRoute> toAdd;
    std::vector<IpPrefix> toDel;
  };

 public:
  using RouterIDAndClient = std::pair<RouterID, ClientID>;
  virtual ~RouteUpdateWrapper() = default;
  using UpdateStatistics = RoutingInformationBase::UpdateStatistics;
  using FibUpdateFunction = RoutingInformationBase::FibUpdateFunction;
  void addRoute(
      RouterID id,
      const folly::IPAddress& network,
      uint8_t mask,
      ClientID clientId,
      RouteNextHopEntry entry);

  void addRoute(RouterID id, ClientID clientId, const UnicastRoute& route);
  void delRoute(
      RouterID id,
      const folly::IPAddress& network,
      uint8_t mask,
      ClientID clientId);

  void delRoute(RouterID id, const IpPrefix& pfx, ClientID clientId);
  void program(const std::unordered_set<RouterIDAndClient>& syncFibFor = {});
  void programMinAlpmState();
  void programClassID(
      RouterID rid,
      const std::vector<folly::CIDRNetwork>& prefixes,
      std::optional<cfg::AclLookupClass> classId,
      bool async);

 private:
  virtual RoutingInformationBase* getRib() = 0;
  virtual void programLegacyRib() = 0;
  void printStats(const UpdateStatistics& stats) const;
  void programStandAloneRib();
  virtual void updateStats(const UpdateStatistics& stats) = 0;
  virtual AdminDistance clientIdToAdminDistance(ClientID clientID) const = 0;
  virtual void programClassIDLegacyRib(
      RouterID rid,
      const std::vector<folly::CIDRNetwork>& prefixes,
      std::optional<cfg::AclLookupClass> classId,
      bool async) = 0;
  void programClassIDStandAloneRib(
      RouterID rid,
      const std::vector<folly::CIDRNetwork>& prefixes,
      std::optional<cfg::AclLookupClass> classId,
      bool async);

 protected:
  std::shared_ptr<SwitchState> updateClassIdLegacyRibHelper(
      const std::shared_ptr<SwitchState>& in,
      RouterID rid,
      const std::vector<folly::CIDRNetwork>& prefixes,
      std::optional<cfg::AclLookupClass> classId);
  std::pair<std::shared_ptr<SwitchState>, UpdateStatistics>
  programLegacyRibHelper(const std::shared_ptr<SwitchState>& in) const;
  RouteUpdateWrapper(
      bool isStandaloneRibEnabled,
      std::optional<FibUpdateFunction> fibUpdateFn,
      void* fibUpdateCookie)
      : isStandaloneRibEnabled_(isStandaloneRibEnabled),
        fibUpdateFn_(fibUpdateFn),
        fibUpdateCookie_(fibUpdateCookie) {}

  std::unordered_map<std::pair<RouterID, ClientID>, AddDelRoutes>
      ribRoutesToAddDel_;
  std::unordered_set<RouterIDAndClient> syncFibFor_;
  bool isStandaloneRibEnabled_{false};
  std::optional<FibUpdateFunction> fibUpdateFn_;
  void* fibUpdateCookie_{nullptr};
};
} // namespace facebook::fboss
