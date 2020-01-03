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

#include <folly/FBString.h>
#include <folly/IPAddress.h>
#include <folly/dynamic.h>

#include <boost/container/flat_map.hpp>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

/**
 * Class relationship:
 *
 * RouteNextHopsMulti is a map from client ID to RouteNextHopEntry.
 *
 * There are two types of RouteNextHopEntry in general.
 * 1) An entry with multiple nexthop IPs (RouteNextHopSet), OR
 * 2) no nexthop IPs, but with pre-defined forwarding action
 *    (i.e. ToCPU or DROP)
 * Each RouteNextHopEntry also has other common attribute, like admin distance.
 *
 * RouteNextHopSet is a set of RouteNextHop.
 *
 * RouteNextHop contains the nexthop IP address. In case of the v6 link-local
 * IP address, the interface ID is also included in this class.
 *
 */

/**
 * Map form clientId -> RouteNextHopEntry
 */
class RouteNextHopsMulti {
 protected:
  ClientID findLowestAdminDistance();
  boost::container::flat_map<ClientID, RouteNextHopEntry> map_;
  ClientID lowestAdminDistanceClientId_;

 public:
  folly::dynamic toFollyDynamic() const;
  static RouteNextHopsMulti fromFollyDynamic(const folly::dynamic& json);

  std::vector<ClientAndNextHops> toThrift() const;

  std::string str() const;
  void update(ClientID clientid, RouteNextHopEntry nhe);

  void clear() {
    map_.clear();
  }

  bool operator==(const RouteNextHopsMulti& p2) const {
    return map_ == p2.map_;
  }

  bool isEmpty() const {
    // The code disallows adding/updating an empty nextHops list. So if the
    // map contains any entries, they are non-zero-length lists.
    return map_.empty();
  }

  void delEntryForClient(ClientID clientId);

  const RouteNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const;

  std::pair<ClientID, const RouteNextHopEntry*> getBestEntry() const;

  bool isSame(ClientID clientId, const RouteNextHopEntry& nhe) const;
};

} // namespace facebook::fboss
