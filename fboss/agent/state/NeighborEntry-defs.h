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

#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/types.h"

#include <sstream>

namespace {
constexpr auto kIpAddr = "ipaddress";
constexpr auto kMac = "mac";
constexpr auto kPort = "portId";
constexpr auto kInterface = "interfaceId";
constexpr auto kNeighborEntryState = "state";
constexpr auto kClassID = "classID";
} // namespace

namespace facebook::fboss {

template <typename IPADDR>
folly::dynamic NeighborEntryFields<IPADDR>::toFollyDynamic() const {
  folly::dynamic entry = folly::dynamic::object;
  entry[kIpAddr] = ip.str();
  entry[kMac] = mac.toString();
  entry[kPort] = port.toFollyDynamic();
  entry[kInterface] = static_cast<uint32_t>(interfaceID);
  entry[kNeighborEntryState] = static_cast<int>(state);
  if (classID.has_value()) {
    entry[kClassID] = static_cast<int>(classID.value());
  }

  return entry;
}

template <typename IPADDR>
NeighborEntryFields<IPADDR> NeighborEntryFields<IPADDR>::fromFollyDynamic(
    const folly::dynamic& entryJson) {
  IPADDR ip(entryJson[kIpAddr].stringPiece());
  folly::MacAddress mac(entryJson[kMac].stringPiece());
  auto port = PortDescriptor::fromFollyDynamic(entryJson[kPort]);
  InterfaceID intf(entryJson[kInterface].asInt());
  auto state = NeighborState(entryJson[kNeighborEntryState].asInt());

  if (entryJson.find(kClassID) != entryJson.items().end()) {
    auto classID = cfg::AclLookupClass(entryJson[kClassID].asInt());
    return NeighborEntryFields(ip, mac, port, intf, state, classID);
  } else {
    return NeighborEntryFields(ip, mac, port, intf, state);
  }
}

template <typename IPADDR, typename SUBCLASS>
NeighborEntry<IPADDR, SUBCLASS>::NeighborEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    InterfaceID interfaceID,
    NeighborState state)
    : Parent(ip, mac, port, interfaceID, state) {}

template <typename IPADDR, typename SUBCLASS>
NeighborEntry<IPADDR, SUBCLASS>::NeighborEntry(
    AddressType ip,
    InterfaceID intfID,
    NeighborState ignored)
    : Parent(ip, intfID, ignored) {}

template <typename IPADDR, typename SUBCLASS>
std::string NeighborEntry<IPADDR, SUBCLASS>::str() const {
  std::ostringstream os;

  auto classIDStr = getClassID().has_value()
      ? folly::to<std::string>(static_cast<int>(getClassID().value()))
      : "None";
  auto neighborStateStr =
      isReachable() ? "Reachable" : (isPending() ? "Pending" : "Unverified");

  os << "NeighborEntry:: MAC: " << getMac().toString()
     << " IP: " << getIP().str() << " classID: " << classIDStr << " "
     << getPort().str() << " NeighborState: " << neighborStateStr;

  return os.str();
}

} // namespace facebook::fboss
