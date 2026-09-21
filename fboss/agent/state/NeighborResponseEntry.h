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

#include <folly/MacAddress.h>
#include <memory>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"

namespace facebook::fboss {

template <typename IPADDR, typename SUBCLASS>
class NeighborResponseEntry
    : public ThriftStructNode<SUBCLASS, state::NeighborResponseEntryFields> {
 public:
  using AddressType = IPADDR;

  NeighborResponseEntry(
      AddressType ipAddress,
      folly::MacAddress mac,
      InterfaceID interfaceID) {
    setIP(ipAddress);
    setMac(mac);
    setInterfaceID(interfaceID);
  }

  std::string getID() const {
    auto ip = getIP();
    return ip.str();
  }

  AddressType getIP() const {
    return AddressType(
        this->template get<switch_state_tags::ipAddress>()->cref());
  }

  void setIP(AddressType ip) {
    this->template set<switch_state_tags::ipAddress>(ip.str());
  }

  folly::MacAddress getMac() const {
    return folly::MacAddress(
        this->template get<switch_state_tags::mac>()->cref());
  }

  void setMac(folly::MacAddress mac) {
    this->template set<switch_state_tags::mac>(mac.toString());
  }

  InterfaceID getInterfaceID() const {
    return InterfaceID(
        this->template get<switch_state_tags::interfaceId>()->cref());
  }

  void setInterfaceID(InterfaceID interface) {
    this->template set<switch_state_tags::interfaceId>(interface);
  }

 private:
  using Parent = ThriftStructNode<SUBCLASS, state::NeighborResponseEntryFields>;
  using Parent::Parent;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
