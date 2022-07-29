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
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/StateUtils.h"

namespace {
constexpr auto kNexthopDelim = "@";
}

namespace facebook::fboss {

//
// RouteNextHop Class
//

folly::dynamic RouteNextHopsMulti::toFollyDynamicLegacy() const {
  // Store the clientid->RouteNextHopEntry map as a dynamic::object
  folly::dynamic obj = folly::dynamic::object();
  for (auto const& row : map_) {
    int clientid = static_cast<int>(row.first);
    const auto& entry = row.second;
    obj[folly::to<std::string>(clientid)] = entry.toFollyDynamicLegacy();
  }
  return obj;
}

RouteNextHopsMulti RouteNextHopsMulti::fromFollyDynamicLegacy(
    const folly::dynamic& json) {
  RouteNextHopsMulti nh;
  for (const auto& pair : json.items()) {
    nh.update(
        ClientID(pair.first.asInt()),
        RouteNextHopEntry::fromFollyDynamicLegacy(pair.second));
  }
  return nh;
}

std::vector<ClientAndNextHops> RouteNextHopsMulti::toThriftLegacy() const {
  std::vector<ClientAndNextHops> list;
  for (const auto& srcPair : map_) {
    ClientAndNextHops destPair;
    *destPair.clientId() = static_cast<int>(srcPair.first);
    for (const auto& nh : srcPair.second.getNextHopSet()) {
      destPair.nextHops()->push_back(nh.toThrift());
    }
    list.push_back(destPair);
  }
  return list;
}

std::string RouteNextHopsMulti::strLegacy() const {
  std::string ret = "";
  for (auto const& row : map_) {
    ClientID clientid = row.first;
    RouteNextHopSet const& nxtHps = row.second.getNextHopSet();

    ret.append(folly::to<std::string>("(client#", clientid, ": "));
    for (const auto& nh : nxtHps) {
      ret.append(folly::to<std::string>(nh.str(), ", "));
    }
    ret.append(")");
  }
  return ret;
}

void RouteNextHopsMulti::update(ClientID clientId, RouteNextHopEntry nhe) {
  auto iter = map_.find(clientId);
  if (iter == map_.end()) {
    map_.insert(std::make_pair(clientId, std::move(nhe)));
  } else {
    iter->second = std::move(nhe);
  }

  // Let's check whether this has a preferred admin distance
  if (map_.size() == 1) {
    setLowestAdminDistanceClientId(clientId);
    return;
  }
  auto entry = getEntryForClient(lowestAdminDistanceClientId());
  if (!entry) {
    setLowestAdminDistanceClientId(findLowestAdminDistance());
  } else if (nhe.getAdminDistance() < entry->getAdminDistance()) {
    // Arbritary choice to use the newest one if we have multiple
    // with the same admin distance
    setLowestAdminDistanceClientId(clientId);
  }
}

ClientID RouteNextHopsMulti::findLowestAdminDistance() {
  if (map_.size() == 0) {
    // We'll set it on the next add
    return ClientID(-1);
  }
  auto lowest = map_.begin();
  auto it = map_.begin();
  while (it != map_.end()) {
    if (it->second.getAdminDistance() < lowest->second.getAdminDistance()) {
      lowest = it;
    } else if (it->second.isSame(lowest->second) && it->first < lowest->first) {
      // If we have two clients with the same admin distance, pick the lowest
      // client id - this is an arbritary, but deterministic, choice
      lowest = it;
    }
    ++it;
  }
  return lowest->first;
}

void RouteNextHopsMulti::delEntryForClient(ClientID clientId) {
  map_.erase(clientId);

  // Let's regen the next best entry
  if (lowestAdminDistanceClientId() == clientId) {
    setLowestAdminDistanceClientId(findLowestAdminDistance());
  }
}

const RouteNextHopEntry* RouteNextHopsMulti::getEntryForClient(
    ClientID clientId) const {
  auto iter = map_.find(clientId);
  if (iter == map_.end()) {
    return nullptr;
  }
  return &iter->second;
}

bool RouteNextHopsMulti::isSame(ClientID id, const RouteNextHopEntry& nhe)
    const {
  auto entry = getEntryForClient(id);
  return entry && (*entry == nhe);
}

std::pair<ClientID, const RouteNextHopEntry*> RouteNextHopsMulti::getBestEntry()
    const {
  auto entry = getEntryForClient(lowestAdminDistanceClientId());
  if (entry) {
    return std::make_pair(lowestAdminDistanceClientId(), entry);
  } else {
    // Throw an exception if the map is empty. Shall not happen
    throw FbossError("Unexpected empty RouteNextHopsMulti");
  }
}

state::RouteNextHopsMulti RouteNextHopsMulti::toThrift() const {
  state::RouteNextHopsMulti thriftMulti{};
  thriftMulti.lowestAdminDistanceClientId() = lowestAdminDistanceClientId();
  for (auto entry : map_) {
    thriftMulti.client2NextHopEntry()->emplace(
        entry.first, entry.second.toThrift());
  }
  return thriftMulti;
}

RouteNextHopsMulti RouteNextHopsMulti::fromThrift(
    const state::RouteNextHopsMulti& multi) {
  RouteNextHopsMulti routeNextHopMulti{};
  routeNextHopMulti.setLowestAdminDistanceClientId(
      *multi.lowestAdminDistanceClientId());
  for (auto entry : *multi.client2NextHopEntry()) {
    routeNextHopMulti.map_.emplace(
        entry.first, RouteNextHopEntry::fromThrift(entry.second));
  }
  return routeNextHopMulti;
}

folly::dynamic RouteNextHopsMulti::migrateToThrifty(folly::dynamic const& dyn) {
  folly::dynamic newDyn = folly::dynamic::dynamic::object;
  folly::dynamic client2NextHopEntryDyn = folly::dynamic::object;
  auto multi = fromFollyDynamicLegacy(dyn);
  for (auto [key, value] : dyn.items()) {
    client2NextHopEntryDyn[key] = RouteNextHopEntry::migrateToThrifty(value);
  }
  newDyn["client2NextHopEntry"] = client2NextHopEntryDyn;
  newDyn["lowestAdminDistanceClientId"] =
      static_cast<int>(multi.lowestAdminDistanceClientId());
  return newDyn;
}

void RouteNextHopsMulti::migrateFromThrifty(folly::dynamic& dyn) {
  for (auto [key, value] : dyn["client2NextHopEntry"].items()) {
    auto clientID = key.asString();
    auto multiDynamic = value;
    RouteNextHopEntry::migrateFromThrifty(multiDynamic);
    dyn[clientID] = multiDynamic;
  }
  dyn.erase("client2NextHopEntry");
  dyn.erase("lowestAdminDistanceClientId");
}

} // namespace facebook::fboss
