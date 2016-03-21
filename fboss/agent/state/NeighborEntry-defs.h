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

namespace facebook { namespace fboss {

template<typename IPADDR>
folly::dynamic NeighborEntryFields<IPADDR>::toFollyDynamic() const {
  folly::dynamic entry = folly::dynamic::object;
  entry[kIpAddr] = ip.str();
  entry[kMac] = mac.toString();
  entry[kPort] = static_cast<uint16_t>(port);
  entry[kInterface] = static_cast<uint32_t>(interfaceID);
  return entry;
}

template<typename IPADDR>
NeighborEntryFields<IPADDR>
NeighborEntryFields<IPADDR>::fromFollyDynamic(
    const folly::dynamic& entryJson) {
  IPADDR ip(entryJson[kIpAddr].stringPiece());
  folly::MacAddress mac(entryJson[kMac].stringPiece());
  PortID port(entryJson[kPort].asInt());
  InterfaceID intf(entryJson[kInterface].asInt());
  // Any entry we deserialize should come back in the UNVERIFIED state
  return NeighborEntryFields(ip, mac, port, intf, NeighborState::UNVERIFIED);
}

template<typename IPADDR, typename SUBCLASS>
NeighborEntry<IPADDR, SUBCLASS>::NeighborEntry(AddressType ip,
                                     folly::MacAddress mac,
                                     PortID port,
                                     InterfaceID interfaceID,
                                     NeighborState state)
  : Parent(ip, mac, port, interfaceID, state) {}

template<typename IPADDR, typename SUBCLASS>
NeighborEntry<IPADDR, SUBCLASS>::NeighborEntry(AddressType ip,
                                     InterfaceID intfID,
                                     NeighborState ignored)
  : Parent(ip, intfID, ignored) {}


}} // facebook::fboss
