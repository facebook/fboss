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
#include "fboss/agent/state/NodeBase.h"

namespace facebook { namespace fboss {

using folly::MacAddress;

enum PendingEntry { PENDING };

template<typename IPADDR>
struct NeighborEntryFields {
  typedef IPADDR AddressType;

  NeighborEntryFields(AddressType ip,
                      folly::MacAddress mac,
                      PortID port,
                      InterfaceID interfaceID)
    : ip(ip),
      mac(mac),
      port(port),
      interfaceID(interfaceID),
      pending(false) {}

  NeighborEntryFields(AddressType ip,
                      InterfaceID interfaceID,
                      PendingEntry ignored)
    : ip(ip),
      mac(MacAddress::BROADCAST),
      port(PortID(0)),
      interfaceID(interfaceID),
      pending(true) {}

  template<typename Fn>
  void forEachChild(Fn fn) {}
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  /*
   * Deserialize from folly::dynamic
   */
  static NeighborEntryFields fromFollyDynamic(const folly::dynamic& entryJson);

  static constexpr auto kIpAddr = "ipaddress";
  static constexpr auto kMac = "mac";
  static constexpr auto kPort = "portId";
  static constexpr auto kInterface = "interfaceId";
  AddressType ip;
  folly::MacAddress mac;
  PortID port;
  InterfaceID interfaceID;
  bool pending;
};

template<typename IPADDR, typename SUBCLASS>
class NeighborEntry : public NodeBaseT<SUBCLASS, NeighborEntryFields<IPADDR>> {
 public:
  typedef IPADDR AddressType;

  NeighborEntry(AddressType ip, folly::MacAddress mac,
                PortID port, InterfaceID interfaceID);

  NeighborEntry(AddressType ip, InterfaceID intfID, PendingEntry ignored);

  static std::shared_ptr<SUBCLASS>
  fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = NeighborEntryFields<IPADDR>::fromFollyDynamic(json);
    return std::make_shared<SUBCLASS>(fields);
  }

  static std::shared_ptr<SUBCLASS>
  fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return this->getFields()->toFollyDynamic();
  }

  AddressType getIP() const {
    return this->getFields()->ip;
  }

  folly::MacAddress getMac() const {
    return this->getFields()->mac;
  }
  void setMAC(folly::MacAddress mac) {
    this->writableFields()->mac = mac;
  }

  PortID getPort() const {
    return this->getFields()->port;
  }

  bool nonZeroPort() const {
    return getPort() != PortID(0);
  }
  bool zeroPort() const {
    return !nonZeroPort();
  }

  void setPort(PortID port) {
    this->writableFields()->port = port;
  }

  InterfaceID getIntfID() const {
    return this->getFields()->interfaceID;
  }
  void setIntfID(InterfaceID id) {
    this->writableFields()->interfaceID = id;
  }

  bool isPending() const {
    return this->getFields()->pending;
  }

  void setPending(bool pending) {
    this->writableFields()->pending = pending;
  }

 private:
  typedef NodeBaseT<SUBCLASS, NeighborEntryFields<IPADDR>> Parent;
  // Inherit the constructors required for clone()
  using Parent::Parent;
  friend class CloneAllocator;
};

}} // facebook::fboss
