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
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"

namespace facebook::fboss {

template <typename IPADDR>
struct NeighborResponseEntryFields : public ThriftyFields {
  using AddressType = IPADDR;
  static constexpr auto kMac = "mac";
  static constexpr auto kNeighborResponseIntf = "interfaceId";

  NeighborResponseEntryFields(
      AddressType ipAddress,
      folly::MacAddress mac,
      InterfaceID interfaceID)
      : ipAddress(ipAddress), mac(mac), interfaceID(interfaceID) {}

  NeighborResponseEntryFields(folly::MacAddress mac, InterfaceID interfaceID)
      : mac(mac), interfaceID(interfaceID) {}

  state::NeighborResponseEntryFields toThrift() const;
  static NeighborResponseEntryFields fromThrift(
      state::NeighborResponseEntryFields const& entryTh);

  folly::dynamic toFollyDynamicLegacy() const;
  static NeighborResponseEntryFields fromFollyDynamicLegacy(
      const folly::dynamic& entry);

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  bool operator==(const NeighborResponseEntryFields& other) const {
    return mac == other.mac && interfaceID == other.interfaceID;
  }

  AddressType ipAddress;
  folly::MacAddress mac;
  InterfaceID interfaceID{0};
};

template <typename IPADDR, typename SUBCLASS>
class NeighborResponseEntry : public ThriftyBaseT<
                                  state::NeighborResponseEntryFields,
                                  SUBCLASS,
                                  NeighborResponseEntryFields<IPADDR>> {
 public:
  using AddressType = IPADDR;

  AddressType getID() const {
    return this->getFields()->ipAddress;
  }

  AddressType getIP() const {
    return this->getFields()->ipAddress;
  }

  void setIP(AddressType ip) {
    this->writableFields()->ipAddress = ip;
  }

  folly::MacAddress getMac() const {
    return this->getFields()->mac;
  }

  void setMac(folly::MacAddress mac) {
    this->writableFields()->mac = mac;
  }

  InterfaceID getInterfaceID() const {
    return this->getFields()->interfaceID;
  }

  void setInterfaceID(InterfaceID interface) {
    this->writableFields()->interfaceID = interface;
  }

 private:
  using Parent = ThriftyBaseT<
      state::NeighborResponseEntryFields,
      SUBCLASS,
      NeighborResponseEntryFields<IPADDR>>;
  using Parent::Parent;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
#include "fboss/agent/state/NeighborResponseEntry-defs.h"
