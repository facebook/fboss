/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/RouteNextHopsMulti.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateUtils.h"

namespace {
constexpr auto kNexthopDelim = "@";
}

namespace facebook::fboss {

//
// RouteNextHop Class
//

std::vector<ClientAndNextHops> RouteNextHopsMulti::toThriftLegacy() const {
  std::vector<ClientAndNextHops> list;
  auto mapRef = map();
  for (const auto& srcPair : *mapRef) {
    ClientAndNextHops destPair;
    *destPair.clientId() = static_cast<int>(srcPair.first);
    for (const auto& nh : srcPair.second->getNextHopSet()) {
      destPair.nextHops()->push_back(nh.toThrift());
    }
    list.push_back(destPair);
  }
  return list;
}

std::string RouteNextHopsMulti::strLegacy() const {
  std::string ret = "";
  auto mapRef = map();
  for (auto const& row : *mapRef) {
    ClientID clientid = row.first;
    const auto& entry = row.second;
    RouteNextHopSet const& nxtHps = entry->getNextHopSet();

    ret.append(folly::to<std::string>("(client#", clientid, ": "));
    for (const auto& nh : nxtHps) {
      ret.append(folly::to<std::string>(nh.str(), ", "));
    }
    ret.append(")");
  }
  return ret;
}

// THRIFT_COPY
void RouteNextHopsMulti::update(
    ClientID clientId,
    const RouteNextHopEntry& nhe) {
  auto data = this->toThrift();
  RouteNextHopsMulti::update(clientId, data, nhe.toThrift());
  this->fromThrift(data);
}

// THRIFT_COPY
ClientID RouteNextHopsMulti::findLowestAdminDistance() {
  return RouteNextHopsMulti::findLowestAdminDistance(toThrift());
}

void RouteNextHopsMulti::delEntryForClient(ClientID clientId) {
  auto data = this->toThrift();
  RouteNextHopsMulti::delEntryForClient(clientId, data);
  this->fromThrift(data);
}

// THRIFT_COPY
std::shared_ptr<const RouteNextHopEntry> RouteNextHopsMulti::getEntryForClient(
    ClientID clientId) const {
  return RouteNextHopsMulti::getEntryForClient(clientId, toThrift());
}

bool RouteNextHopsMulti::isSame(ClientID id, const RouteNextHopEntry& nhe)
    const {
  auto entry = getEntryForClient(id);
  return entry && (*entry == nhe);
}

// THRIFT_COPY
std::pair<ClientID, std::shared_ptr<const RouteNextHopEntry>>
RouteNextHopsMulti::getBestEntry() const {
  return RouteNextHopsMulti::getBestEntry(toThrift());
}

std::pair<ClientID, std::shared_ptr<const RouteNextHopEntry>>
RouteNextHopsMulti::getBestEntry(
    const state::RouteNextHopsMulti& nexthopsmulti) {
  ClientID clientID = *nexthopsmulti.lowestAdminDistanceClientId();
  auto entry = RouteNextHopsMulti::getEntryForClient(clientID, nexthopsmulti);
  if (entry) {
    return std::make_pair(clientID, std::move(entry));
  } else {
    // Throw an exception if the map is empty. Shall not happen
    throw FbossError("Unexpected empty RouteNextHopsMulti");
  }
}

std::shared_ptr<const RouteNextHopEntry> RouteNextHopsMulti::getEntryForClient(
    ClientID clientId,
    const state::RouteNextHopsMulti& nexthopsmulti) {
  const auto& map = *(nexthopsmulti.client2NextHopEntry());
  auto iter = map.find(clientId);
  if (iter == map.end()) {
    return nullptr;
  }
  return std::make_shared<const RouteNextHopEntry>(iter->second);
}

void RouteNextHopsMulti::update(
    ClientID clientId,
    state::RouteNextHopsMulti& nexthopsmulti,
    state::RouteNextHopEntry nhe) {
  auto& map = *(nexthopsmulti.client2NextHopEntry());
  auto iter = map.find(clientId);
  if (iter == map.end()) {
    map.insert(std::make_pair(clientId, nhe));
  } else {
    iter->second = nhe;
  }

  // Let's check whether this has a preferred admin distance
  if (map.size() == 1) {
    nexthopsmulti.lowestAdminDistanceClientId() = clientId;
    return;
  }
  auto lowestAdminDistanceClientId =
      *nexthopsmulti.lowestAdminDistanceClientId();
  auto entry = RouteNextHopsMulti::getEntryForClient(
      lowestAdminDistanceClientId, nexthopsmulti);
  if (!entry) {
    nexthopsmulti.lowestAdminDistanceClientId() =
        RouteNextHopsMulti::findLowestAdminDistance(nexthopsmulti);
  } else if (*(nhe.adminDistance()) < entry->getAdminDistance()) {
    // Arbritary choice to use the newest one if we have multiple
    // with the same admin distance
    nexthopsmulti.lowestAdminDistanceClientId() = clientId;
  }
}

ClientID RouteNextHopsMulti::findLowestAdminDistance(
    const state::RouteNextHopsMulti& nexthopsmulti) {
  const auto& map = *nexthopsmulti.client2NextHopEntry();
  if (map.size() == 0) {
    // We'll set it on the next add
    return ClientID(-1);
  }
  auto lowest = map.begin();
  auto it = map.begin();
  while (it != map.end()) {
    if (*(it->second.adminDistance()) < *(lowest->second.adminDistance())) {
      lowest = it;
    } else if (it->second == lowest->second && it->first < lowest->first) {
      // If we have two clients with the same admin distance, pick the lowest
      // client id - this is an arbritary, but deterministic, choice
      lowest = it;
    }
    ++it;
  }
  return lowest->first;
}

void RouteNextHopsMulti::delEntryForClient(
    ClientID clientId,
    state::RouteNextHopsMulti& nexthopsmulti) {
  auto& map = *(nexthopsmulti.client2NextHopEntry());
  map.erase(clientId);
  // Let's regen the next best entry
  if (*nexthopsmulti.lowestAdminDistanceClientId() == clientId) {
    nexthopsmulti.lowestAdminDistanceClientId() =
        RouteNextHopsMulti::findLowestAdminDistance(nexthopsmulti);
  }
}

std::shared_ptr<RouteNextHopsMulti> RouteNextHopsMulti::fromFollyDynamic(
    const folly::dynamic& json) {
  auto legacy = LegacyRouteNextHopsMulti::fromFollyDynamic(json);
  return std::make_shared<RouteNextHopsMulti>(legacy.toThrift());
}

folly::dynamic RouteNextHopsMulti::toFollyDynamic() const {
  LegacyRouteNextHopsMulti legacy(this->toThrift());
  return legacy.toFollyDynamic();
}
} // namespace facebook::fboss
