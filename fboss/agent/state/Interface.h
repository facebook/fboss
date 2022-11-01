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

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <folly/dynamic.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>

namespace facebook::fboss {

class SwitchState;

struct InterfaceFields
    : public ThriftyFields<InterfaceFields, state::InterfaceFields> {
  using ThriftyFields::ThriftyFields;
  typedef boost::container::flat_map<folly::IPAddress, uint8_t> Addresses;
  InterfaceFields(
      InterfaceID id,
      RouterID router,
      std::optional<VlanID> vlan,
      folly::StringPiece name,
      folly::MacAddress mac,
      int mtu,
      bool isVirtual,
      bool isStateSyncDisabled,
      cfg::InterfaceType type = cfg::InterfaceType::VLAN) {
    writableData().interfaceId() = id;
    writableData().routerId() = router;
    writableData().type() = type;
    if (type == cfg::InterfaceType::VLAN) {
      CHECK(vlan) << " Vlan ID must be set for interface types VLAN";
    }
    if (vlan) {
      writableData().vlanId() = *vlan;
    }
    writableData().name() = name;
    writableData().mac() = mac.u64NBO();
    writableData().mtu() = mtu;
    writableData().isVirtual() = isVirtual;
    writableData().isStateSyncDisabled() = isStateSyncDisabled;
  }

  state::InterfaceFields toThrift() const override {
    return data();
  }
  static InterfaceFields fromThrift(
      state::InterfaceFields const& interfaceFields) {
    return InterfaceFields(interfaceFields);
  }
  bool operator==(const InterfaceFields& other) const {
    return data() == other.data();
  }

  /*
   * Deserialize from a folly::dynamic object
   */
  static InterfaceFields fromFollyDynamic(const folly::dynamic& json);
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  template <typename Fn>
  void forEachChild(Fn /*fn*/) {}
};

/*
 * Interface stores a routing domain on the switch
 */
class Interface : public NodeBaseT<Interface, InterfaceFields> {
 public:
  using ThriftType = state::InterfaceFields;

  typedef InterfaceFields::Addresses Addresses;
  Interface(
      InterfaceID id,
      RouterID router,
      std::optional<VlanID> vlan,
      folly::StringPiece name,
      folly::MacAddress mac,
      int mtu,
      bool isVirtual,
      bool isStateSyncDisabled,
      cfg::InterfaceType type = cfg::InterfaceType::VLAN)
      : NodeBaseT(
            id,
            router,
            vlan,
            name,
            mac,
            mtu,
            isVirtual,
            isStateSyncDisabled,
            type) {}

  InterfaceID getID() const {
    return InterfaceID(*getFields()->data().interfaceId());
  }

  RouterID getRouterID() const {
    return RouterID(*getFields()->data().routerId());
  }
  void setRouterID(RouterID id) {
    writableFields()->writableData().routerId() = id;
  }
  cfg::InterfaceType getType() const {
    return *getFields()->data().type();
  }
  void setType(cfg::InterfaceType type) {
    writableFields()->writableData().type() = type;
  }
  VlanID getVlanID() const {
    CHECK(getType() == cfg::InterfaceType::VLAN);
    return VlanID(*getFields()->data().vlanId());
  }
  std::optional<VlanID> getVlanIDIf() const {
    if (getType() == cfg::InterfaceType::VLAN) {
      return VlanID(*getFields()->data().vlanId());
    } else {
      return std::nullopt;
    }
  }
  void setVlanID(VlanID id) {
    CHECK(getType() == cfg::InterfaceType::VLAN);
    writableFields()->writableData().vlanId() = id;
  }

  int getMtu() const {
    return *getFields()->data().mtu();
  }
  void setMtu(int mtu) {
    writableFields()->writableData().mtu() = mtu;
  }

  const std::string& getName() const {
    return *getFields()->data().name();
  }
  void setName(const std::string& name) {
    writableFields()->writableData().name() = name;
  }

  folly::MacAddress getMac() const {
    return folly::MacAddress::fromNBO(*getFields()->data().mac());
  }
  void setMac(folly::MacAddress mac) {
    writableFields()->writableData().mac() = mac.u64NBO();
  }

  bool isVirtual() const {
    return *getFields()->data().isVirtual();
  }
  void setIsVirtual(bool isVirtual) {
    writableFields()->writableData().isVirtual() = isVirtual;
  }

  bool isStateSyncDisabled() const {
    return *getFields()->data().isStateSyncDisabled();
  }
  void setIsStateSyncDisabled(bool isStateSyncDisabled) {
    writableFields()->writableData().isStateSyncDisabled() =
        isStateSyncDisabled;
  }

  template <typename AddressType>
  const state::NeighborEntries& getNeighborEntryTable() const {
    if constexpr (std::is_same_v<AddressType, folly::IPAddressV4>) {
      return getArpTable();
    }
    return getNdpTable();
  }

  const state::NeighborEntries& getArpTable() const {
    return *getFields()->data().arpTable();
  }
  const state::NeighborEntries& getNdpTable() const {
    return *getFields()->data().ndpTable();
  }
  template <typename AddressType>
  void setNeighborEntryTable(state::NeighborEntries nbrTable) {
    if constexpr (std::is_same_v<AddressType, folly::IPAddressV4>) {
      return setArpTable(std::move(nbrTable));
    }
    return setNdpTable(std::move(nbrTable));
  }
  void setArpTable(state::NeighborEntries arpTable) {
    writableFields()->writableData().arpTable() = std::move(arpTable);
  }
  void setNdpTable(state::NeighborEntries ndpTable) {
    writableFields()->writableData().ndpTable() = std::move(ndpTable);
  }
  Addresses getAddresses() const {
    Addresses addresses;
    for (const auto& [ipStr, mask] : *getFields()->data().addresses()) {
      addresses.emplace(folly::CIDRNetwork(ipStr, mask));
    }
    return addresses;
  }
  void setAddresses(Addresses addrs) {
    std::map<std::string, int16_t> addresses;
    for (const auto& [addr, mask] : addrs) {
      addresses.emplace(addr.str(), mask);
    }
    writableFields()->writableData().addresses() = std::move(addresses);
  }
  bool hasAddress(folly::IPAddress ip) const {
    auto ipStr = ip.str();
    const auto& addrs = getFields()->data().addresses();
    return addrs->find(ipStr) != addrs->end();
  }

  /**
   * Find the interface IP address to reach the given destination
   */
  std::optional<folly::CIDRNetwork> getAddressToReach(
      const folly::IPAddress& dest) const;

  /**
   * Returns true if the address can be reached through this interface
   */
  bool canReachAddress(const folly::IPAddress& dest) const;

  const cfg::NdpConfig& getNdpConfig() const {
    return *getFields()->data().ndpConfig();
  }
  void setNdpConfig(const cfg::NdpConfig& ndp) {
    writableFields()->writableData().ndpConfig() = ndp;
  }

  /*
   * A utility function to check if an IP address is in a locally attached
   * subnet on the given interface.
   *
   * This a generic helper function.  The Interface class simply seemed like
   * the best place to put it for now.
   */
  static bool isIpAttached(
      folly::IPAddress ip,
      InterfaceID intfID,
      const std::shared_ptr<SwitchState>& state);

  // defer all bellow methods to the fields
  state::InterfaceFields toThrift() const {
    return getFields()->toThrift();
  }
  static std::shared_ptr<Interface> fromThrift(
      state::InterfaceFields const& interfaceFields) {
    return std::make_shared<Interface>(InterfaceFields(interfaceFields));
  }
  bool operator==(const Interface& other) const {
    return *getFields() == *other.getFields();
  }
  static std::shared_ptr<Interface> fromFollyDynamic(
      const folly::dynamic& json) {
    return std::make_shared<Interface>(InterfaceFields::fromFollyDynamic(json));
  }
  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  /*
   * Inherit the constructors required for clone().
   * This needs to be public, as std::make_shared requires
   * operator new() to be available.
   */
  inline static const int kDefaultMtu{1500};

 private:
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
