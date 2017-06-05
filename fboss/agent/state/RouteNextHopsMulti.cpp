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

namespace facebook { namespace fboss {

//
// RouteNextHop Class
//

// Old code has its own format expectation.
// Keep this function until whole fleet is pushed with the code to use
// the new format.
folly::dynamic RouteNextHopsMulti::toFollyDynamicOld() const {
  // Store the clientid->nextHops map as a dynamic::object
  folly::dynamic obj = folly::dynamic::object();
  for (auto const& row : map_) {
    auto clientid = row.first;
    const auto& entry = row.second;
    // The old code expects empty 'nexthopsmulti' for
    // interface and action routes
    if (clientid == StdClientIds2ClientID(StdClientIds::INTERFACE_ROUTE)
        || entry.getAction() != RouteForwardAction::NEXTHOPS) {
      continue;
    }

    RouteNextHops const& nxtHps = entry.getNextHopSet();
    folly::dynamic nxtHopCopy = folly::dynamic::array;
    for (const auto& nhop: nxtHps) {
      std::string intfID = "";
      if (nhop.intfID().hasValue()) {
        intfID = folly::sformat(
            "{}{}", kNexthopDelim,
            static_cast<uint32_t>(nhop.intfID().value()));
      }
      nxtHopCopy.push_back(folly::sformat("{}{}", nhop.addr().str(), intfID));
    }
    obj[folly::to<std::string>(clientid)] = nxtHopCopy;
  }
  return obj;
}

RouteNextHopsMulti
RouteNextHopsMulti::fromFollyDynamicOld(const folly::dynamic& json) {
  RouteNextHopsMulti nh;
  for (const auto& pair: json.items()) {
    int clientId = pair.first.asInt();
    RouteNextHops list;
    for (const auto& ipnh : pair.second) {
      std::vector<std::string> parts;
      folly::split(kNexthopDelim, ipnh.asString(), parts);
      CHECK(0 < parts.size() && parts.size() < 3);

      folly::Optional<InterfaceID> intfID;
      if (parts.size() == 2) {
        intfID = InterfaceID(folly::to<uint32_t>(parts.at(1)));
      }

      list.emplace(
          RouteNextHop::createNextHop(folly::IPAddress(parts[0]), intfID));
    }
    nh.update(ClientID(clientId), RouteNextHopEntry(std::move(list)));
  }
  return nh;
}

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

RouteNextHopsMulti
RouteNextHopsMulti::fromFollyDynamic(const folly::dynamic& json) {
  RouteNextHopsMulti nh;
  for (const auto& pair : json.items()) {
    nh.update(ClientID(pair.first.asInt()),
              RouteNextHopEntry::fromFollyDynamic(pair.second));
  }
  return nh;
}

std::vector<ClientAndNextHops> RouteNextHopsMulti::toThrift() const {
  std::vector<ClientAndNextHops> list;
  for (const auto& srcPair : map_) {
    ClientAndNextHops destPair;
    destPair.clientId = srcPair.first;
    for (const auto& nh : srcPair.second.getNextHopSet()) {
      destPair.nextHopAddrs.push_back(network::toBinaryAddress(nh.addr()));
      if (nh.intfID().hasValue()) {
        auto& nhAddr = destPair.nextHopAddrs.back();
        nhAddr.__isset.ifName = true;
        nhAddr.ifName = util::createTunIntfName(nh.intfID().value());
      }
    }
    list.push_back(destPair);
  }
  return list;
}

std::string RouteNextHopsMulti::str() const {
  std::string ret = "";
  for (auto const & row : map_) {
    int clientid = row.first;
    RouteNextHops const& nxtHps = row.second.getNextHopSet();

    ret.append(folly::to<std::string>("(client#", clientid, ": "));
    for (const auto& nh : nxtHps) {
      ret.append(folly::to<std::string>(nh.addr(), ", "));
    }
    ret.append(")");
  }
  return ret;
}

void RouteNextHopsMulti::update(ClientID clientId, RouteNextHopEntry nhe) {
  map_[clientId] = std::move(nhe);
}

void RouteNextHopsMulti::delEntryForClient(ClientID clientId) {
  map_.erase(clientId);
}

const RouteNextHopEntry* RouteNextHopsMulti::getEntryForClient(
    ClientID clientId) const {
  auto iter = map_.find(clientId);
  if (iter == map_.end()) {
    return nullptr;
  }
  return &iter->second;
}

bool RouteNextHopsMulti::isSame(ClientID id, const RouteNextHops& nhs) const {
  auto entry = getEntryForClient(id);
  return entry && (entry->getNextHopSet() == nhs);
}

std::pair<ClientID, const RouteNextHopEntry *>
RouteNextHopsMulti::getBestEntry() const {
  // I still need to implement a scheme where each clientId
  // can be assigned a priority.  But for now, we're just saying
  // the clientId == its priority.
  //
  // Since flat_map stores items in key-sorted order, the smallest key
  // (and hence the highest-priority client) is first.
  auto iter = map_.begin();
  if (iter != map_.end()) {
    return std::make_pair(iter->first, &iter->second);
  } else {
    // Throw an exception if the map is empty. Shall not happen
    throw FbossError("Unexpected empty RouteNextHopsMulti");
  }
}

}}
