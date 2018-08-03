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

namespace {
constexpr auto kIpAddr = "ipaddress";
constexpr auto kMac = "mac";
constexpr auto kPort = "portId";
constexpr auto kInterface = "interfaceId";
}

namespace facebook { namespace fboss {

template<typename IPADDR>
folly::dynamic NeighborEntryFields<IPADDR>::toFollyDynamic() const {
  folly::dynamic entry = folly::dynamic::object;
  entry[kIpAddr] = ip.str();
  entry[kMac] = mac.toString();
  entry[kPort] = port.toFollyDynamic();
  entry[kInterface] = static_cast<uint32_t>(interfaceID);
  return entry;
}

template<typename IPADDR>
NeighborEntryFields<IPADDR>
NeighborEntryFields<IPADDR>::fromFollyDynamic(
    const folly::dynamic& entryJson) {
  IPADDR ip(entryJson[kIpAddr].stringPiece());
  folly::MacAddress mac(entryJson[kMac].stringPiece());
  auto port = PortDescriptor::fromFollyDynamic(entryJson[kPort]);
  InterfaceID intf(entryJson[kInterface].asInt());
  // Any entry we deserialize should come back in the UNVERIFIED state
  return NeighborEntryFields(ip, mac, port, intf, NeighborState::UNVERIFIED);
}

template<typename IPADDR, typename SUBCLASS>
NeighborEntry<IPADDR, SUBCLASS>::NeighborEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    InterfaceID interfaceID,
    NeighborState state)
    : Parent(ip, mac, port, interfaceID, state) {}

template<typename IPADDR, typename SUBCLASS>
NeighborEntry<IPADDR, SUBCLASS>::NeighborEntry(AddressType ip,
                                     InterfaceID intfID,
                                     NeighborState ignored)
  : Parent(ip, intfID, ignored) {}


}} // facebook::fboss
