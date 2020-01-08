/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteNextHopsMulti.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/StateUtils.h"

namespace {
constexpr auto kNexthopDelim = "@";
}

namespace facebook::fboss::rib {

//
// RouteNextHop Class
//

folly::dynamic RouteNextHopsMulti::toFollyDynamic() const {
  // Store the clientid->RouteNextHopEntry map as a dynamic::object
  folly::dynamic obj = folly::dynamic::object();
  for (auto const& row : map_) {
    int clientid = static_cast<int>(row.first);
    const auto& entry = row.second;
    obj[folly::to<std::string>(clientid)] = entry.toFollyDynamic();
  }
  return obj;
}

RouteNextHopsMulti RouteNextHopsMulti::fromFollyDynamic(
    const folly::dynamic& json) {
  RouteNextHopsMulti nh;
  for (const auto& pair : json.items()) {
    nh.update(
        ClientID(pair.first.asInt()),
        RouteNextHopEntry::fromFollyDynamic(pair.second));
  }
  return nh;
}

std::vector<ClientAndNextHops> RouteNextHopsMulti::toThrift() const {
  std::vector<ClientAndNextHops> list;
  for (const auto& srcPair : map_) {
    ClientAndNextHops destPair;
    destPair.clientId = static_cast<int>(srcPair.first);
    for (const auto& nh : srcPair.second.getNextHopSet()) {
      // Populate `.nextHops` attribute as it is primary one indicating nexthops
      destPair.nextHops.push_back(nh.toThrift());

      // Populate `nextHopAddrs` for backward compatibility
      // TODO: Remove `nextHopAddrs` as protocols (BGP, Open/R) has migrated to
      // using ".nextHops" attribute
      destPair.nextHopAddrs.push_back(network::toBinaryAddress(nh.addr()));
      if (nh.intfID().has_value()) {
        auto& nhAddr = destPair.nextHopAddrs.back();
        nhAddr.ifName_ref() =
            facebook::fboss::util::createTunIntfName(nh.intfID().value());
      }
    }
    list.push_back(destPair);
  }
  return list;
}

std::string RouteNextHopsMulti::str() const {
  std::string ret = "";
  for (auto const& row : map_) {
    ClientID clientid = row.first;
    RouteNextHopSet const& nxtHps = row.second.getNextHopSet();

    ret.append(folly::to<std::string>(
        "(client#", folly::to<std::string>(clientid), ": "));
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
    lowestAdminDistanceClientId_ = clientId;
    return;
  }
  auto entry = getEntryForClient(lowestAdminDistanceClientId_);
  if (!entry) {
    lowestAdminDistanceClientId_ = findLowestAdminDistance();
  } else if (nhe.getAdminDistance() < entry->getAdminDistance()) {
    // Arbritary choice to use the newest one if we have multiple
    // with the same admin distance
    lowestAdminDistanceClientId_ = clientId;
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
  if (lowestAdminDistanceClientId_ == clientId) {
    lowestAdminDistanceClientId_ = findLowestAdminDistance();
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
  auto entry = getEntryForClient(lowestAdminDistanceClientId_);
  if (entry) {
    return std::make_pair(lowestAdminDistanceClientId_, entry);
  } else {
    // Throw an exception if the map is empty. Shall not happen
    throw FbossError("Unexpected empty RouteNextHopsMulti");
  }
}

} // namespace facebook::fboss::rib
