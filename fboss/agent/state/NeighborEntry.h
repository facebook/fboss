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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/Thrifty.h"

namespace facebook::fboss {

using folly::MacAddress;

// TODO: remove this in favor of state::NeighborState in switch_state.thrift
enum class NeighborState { UNVERIFIED, PENDING, REACHABLE };

template <typename IPADDR>
struct NeighborEntryFields : public ThriftyFields {
  typedef IPADDR AddressType;

  NeighborEntryFields(
      AddressType ip,
      folly::MacAddress mac,
      PortDescriptor port,
      InterfaceID interfaceID,
      NeighborState state = NeighborState::REACHABLE,
      std::optional<cfg::AclLookupClass> classID = std::nullopt)
      : ip(ip),
        mac(mac),
        port(port),
        interfaceID(interfaceID),
        state(state),
        classID(classID) {}

  NeighborEntryFields(
      AddressType ip,
      InterfaceID interfaceID,
      NeighborState pending)
      : NeighborEntryFields(
            ip,
            MacAddress::BROADCAST,
            PortDescriptor(PortID(0)),
            interfaceID,
            pending) {
    // This constructor should only be used for PENDING entries
    CHECK(pending == NeighborState::PENDING);
  }

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}

  state::NeighborEntryFields toThrift() const {
    state::NeighborEntryFields entryTh;
    entryTh.ipaddress_ref() = ip.str();
    entryTh.mac_ref() = mac.toString();
    entryTh.portId_ref() = port.toThrift();
    entryTh.interfaceId_ref() = static_cast<uint32_t>(interfaceID);
    entryTh.state_ref() = static_cast<state::NeighborState>(state);
    if (classID.has_value()) {
      entryTh.classID_ref() = classID.value();
    }
    return entryTh;
  }

  static NeighborEntryFields fromThrift(
      state::NeighborEntryFields const& entryTh) {
    IPADDR ip(entryTh.get_ipaddress());
    folly::MacAddress mac(entryTh.get_mac());
    auto port = PortDescriptor::fromThrift(entryTh.get_portId());
    InterfaceID intf(entryTh.get_interfaceId());
    auto state = NeighborState(static_cast<int>(entryTh.get_state()));

    if (entryTh.classID_ref().has_value() && !ip.isLinkLocal()) {
      return NeighborEntryFields(
          ip, mac, port, intf, state, *entryTh.classID_ref());
    } else {
      return NeighborEntryFields(ip, mac, port, intf, state);
    }
  }

  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamicLegacy() const;

  /*
   * Deserialize from folly::dynamic
   */
  static NeighborEntryFields fromFollyDynamicLegacy(
      const folly::dynamic& entryJson);

  AddressType ip;
  folly::MacAddress mac;
  PortDescriptor port;
  InterfaceID interfaceID;
  NeighborState state;
  std::optional<cfg::AclLookupClass> classID{std::nullopt};
};

template <typename IPADDR, typename SUBCLASS>
class NeighborEntry : public ThriftyBaseT<
                          state::NeighborEntryFields,
                          SUBCLASS,
                          NeighborEntryFields<IPADDR>> {
 public:
  typedef IPADDR AddressType;

  NeighborEntry(
      AddressType ip,
      folly::MacAddress mac,
      PortDescriptor port,
      InterfaceID interfaceID,
      NeighborState state = NeighborState::REACHABLE);

  NeighborEntry(AddressType ip, InterfaceID intfID, NeighborState ignored);

  static std::shared_ptr<SUBCLASS> fromFollyDynamicLegacy(
      const folly::dynamic& json) {
    const auto& fields =
        NeighborEntryFields<IPADDR>::fromFollyDynamicLegacy(json);
    return std::make_shared<SUBCLASS>(fields);
  }

  folly::dynamic toFollyDynamicLegacy() const {
    return this->getFields()->toFollyDynamicLegacy();
  }

  bool operator==(const NeighborEntry& other) const {
    return getIP() == other.getIP() && getMac() == other.getMac() &&
        getPort() == other.getPort() && getIntfID() == other.getIntfID() &&
        getState() == other.getState() && getClassID() == other.getClassID();
  }
  bool operator!=(const NeighborEntry& other) const {
    return !operator==(other);
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

  void setPort(PortDescriptor port) {
    this->writableFields()->port = port;
  }
  PortDescriptor getPort() const {
    return this->getFields()->port;
  }

  NeighborState getState() const {
    return this->getFields()->state;
  }

  bool nonZeroPort() const {
    return getPort() != PortID(0);
  }
  bool zeroPort() const {
    return !nonZeroPort();
  }

  InterfaceID getIntfID() const {
    return this->getFields()->interfaceID;
  }
  void setIntfID(InterfaceID id) {
    this->writableFields()->interfaceID = id;
  }

  NeighborState setState(NeighborState state) {
    return this->writableFields()->state = state;
  }

  bool isPending() const {
    return this->getFields()->state == NeighborState::PENDING;
  }

  void setPending() {
    this->writableFields()->state = NeighborState::PENDING;
  }

  bool isReachable() const {
    return this->getFields()->state == NeighborState::REACHABLE;
  }

  std::optional<cfg::AclLookupClass> getClassID() const {
    return this->getFields()->classID;
  }

  void setClassID(std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    this->writableFields()->classID = classID;
  }

  std::string str() const;

 private:
  typedef ThriftyBaseT<
      state::NeighborEntryFields,
      SUBCLASS,
      NeighborEntryFields<IPADDR>>
      Parent;
  // Inherit the constructors required for clone()
  using Parent::Parent;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
