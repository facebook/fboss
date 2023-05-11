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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>

#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpTable.h"

namespace facebook::fboss {

class SwitchState;

// both arp table and ndp table have same thrift type representation as map of
// string to neighbor entry fields. define which of these two members of struct
// resolves to which class.
class Interface;
RESOLVE_STRUCT_MEMBER(Interface, switch_state_tags::arpTable, ArpTable)
RESOLVE_STRUCT_MEMBER(Interface, switch_state_tags::ndpTable, NdpTable)
RESOLVE_STRUCT_MEMBER(
    Interface,
    switch_state_tags::arpResponseTable,
    ArpResponseTable)
RESOLVE_STRUCT_MEMBER(
    Interface,
    switch_state_tags::ndpResponseTable,
    NdpResponseTable)

/*
 * Interface stores a routing domain on the switch
 */
class Interface : public ThriftStructNode<Interface, state::InterfaceFields> {
 public:
  using Base = ThriftStructNode<Interface, state::InterfaceFields>;
  using Base::modify;
  using AddressesType = Base::Fields::TypeFor<switch_state_tags::addresses>;
  using Addresses = std::map<folly::IPAddress, uint8_t>;
  Interface(
      InterfaceID id,
      RouterID router,
      std::optional<VlanID> vlan,
      folly::StringPiece name,
      folly::MacAddress mac,
      int mtu,
      bool isVirtual,
      bool isStateSyncDisabled,
      cfg::InterfaceType type = cfg::InterfaceType::VLAN) {
    set<switch_state_tags::interfaceId>(id);
    setRouterID(router);
    if (vlan) {
      setVlanID(*vlan);
    }
    setName(name.str());
    setMac(mac);
    setMtu(mtu);
    setIsVirtual(isVirtual);
    setIsStateSyncDisabled(isStateSyncDisabled);
    setType(type);
  }

  InterfaceID getID() const {
    return InterfaceID(get<switch_state_tags::interfaceId>()->cref());
  }
  std::optional<SystemPortID> getSystemPortID() const;

  RouterID getRouterID() const {
    return RouterID(get<switch_state_tags::routerId>()->cref());
  }
  void setRouterID(RouterID id) {
    set<switch_state_tags::routerId>(id);
  }
  cfg::InterfaceType getType() const {
    return get<switch_state_tags::type>()->cref();
  }
  void setType(cfg::InterfaceType type) {
    set<switch_state_tags::type>(type);
  }
  VlanID getVlanID() const {
    CHECK(getType() == cfg::InterfaceType::VLAN);
    return VlanID(get<switch_state_tags::vlanId>()->cref());
  }
  std::optional<VlanID> getVlanIDIf() const {
    if (getType() == cfg::InterfaceType::VLAN) {
      return getVlanID();
    } else {
      return std::nullopt;
    }
  }
  void setVlanID(VlanID id) {
    CHECK(getType() == cfg::InterfaceType::VLAN);
    set<switch_state_tags::vlanId>(id);
  }

  VlanID getVlanIDHelper() const {
    // TODO(skhare)
    // VOQ/Fabric switches require that the packets are not tagged with any
    // VLAN. We are gradually enhancing wedge_agent to handle tagged as well as
    // untagged packets. During this transition, we will use VlanID 0 to
    // populate SwitchState/Neighbor cache etc. data structures. Once the
    // wedge_agent changes are complete, we will no longer need this function.
    return getType() == cfg::InterfaceType::VLAN ? getVlanID() : VlanID(0);
  }

  Interface* modify(std::shared_ptr<SwitchState>* state);
  int getMtu() const {
    return get<switch_state_tags::mtu>()->cref();
  }
  void setMtu(int mtu) {
    set<switch_state_tags::mtu>(mtu);
  }

  std::string getName() const {
    return get<switch_state_tags::name>()->cref();
  }
  void setName(const std::string& name) {
    set<switch_state_tags::name>(name);
  }

  folly::MacAddress getMac() const {
    return folly::MacAddress::fromNBO(get<switch_state_tags::mac>()->cref());
  }
  void setMac(folly::MacAddress mac) {
    set<switch_state_tags::mac>(mac.u64NBO());
  }

  bool isVirtual() const {
    return get<switch_state_tags::isVirtual>()->cref();
  }
  void setIsVirtual(bool isVirtual) {
    set<switch_state_tags::isVirtual>(isVirtual);
  }

  bool isStateSyncDisabled() const {
    return get<switch_state_tags::isStateSyncDisabled>()->cref();
  }
  void setIsStateSyncDisabled(bool isStateSyncDisabled) {
    set<switch_state_tags::isStateSyncDisabled>(isStateSyncDisabled);
  }

  template <typename NTableT>
  auto getTable() const {
    if constexpr (std::is_same_v<NTableT, ArpTable>) {
      return getArpTable();
    } else if constexpr (std::is_same_v<NTableT, NdpTable>) {
      return getNdpTable();
    }
  }

  std::shared_ptr<ArpTable> getArpTable() const {
    return get<switch_state_tags::arpTable>();
  }
  std::shared_ptr<NdpTable> getNdpTable() const {
    return get<switch_state_tags::ndpTable>();
  }
  template <
      typename AddressType,
      std::enable_if_t<std::is_same_v<AddressType, folly::IPAddressV4>, bool> =
          true>
  auto getNeighborEntryTable() const {
    return getArpTable();
  }

  template <
      typename AddressType,
      std::enable_if_t<std::is_same_v<AddressType, folly::IPAddressV6>, bool> =
          true>
  auto getNeighborEntryTable() const {
    return getNdpTable();
  }

  template <typename NTable>
  inline const std::shared_ptr<NTable> getNeighborTable() const;

  void setArpTable(state::NeighborEntries arpTable) {
    set<switch_state_tags::arpTable>(std::move(arpTable));
  }
  void setNdpTable(state::NeighborEntries ndpTable) {
    set<switch_state_tags::ndpTable>(std::move(ndpTable));
  }
  template <typename AddressType>
  void setNeighborEntryTable(state::NeighborEntries nbrTable) {
    if constexpr (std::is_same_v<AddressType, folly::IPAddressV4>) {
      return setArpTable(std::move(nbrTable));
    }
    return setNdpTable(std::move(nbrTable));
  }

  const std::shared_ptr<ArpResponseTable> getArpResponseTable() const {
    return get<switch_state_tags::arpResponseTable>();
  }
  void setArpResponseTable(std::shared_ptr<ArpResponseTable> table) {
    ref<switch_state_tags::arpResponseTable>() = std::move(table);
  }

  const std::shared_ptr<NdpResponseTable> getNdpResponseTable() const {
    return get<switch_state_tags::ndpResponseTable>();
  }
  void setNdpResponseTable(std::shared_ptr<NdpResponseTable> table) {
    ref<switch_state_tags::ndpResponseTable>() = std::move(table);
  }

  auto getAddresses() const {
    return get<switch_state_tags::addresses>();
  }
  void setAddresses(Addresses addrs) {
    std::map<std::string, int16_t> addresses;
    for (const auto& [addr, mask] : addrs) {
      addresses.emplace(addr.str(), mask);
    }
    set<switch_state_tags::addresses>(std::move(addresses));
  }
  bool hasAddress(folly::IPAddress ip) const {
    auto& addresses = std::as_const(*getAddresses());
    return (addresses.find(ip.str()) != addresses.end());
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

  auto getNdpConfig() const {
    return get<switch_state_tags::ndpConfig>();
  }
  void setNdpConfig(cfg::NdpConfig ndp) {
    set<switch_state_tags::ndpConfig>(std::move(ndp));
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

  bool operator!=(const Interface& other) const {
    return !(*this == other);
  }

  Addresses getAddressesCopy() const {
    // THRIFT_COPY: evaluate if this function can be retired..
    Addresses addrs{};
    for (auto iter : std::as_const(*getAddresses())) {
      addrs.emplace(folly::IPAddress(iter.first), iter.second->cref());
    }
    return addrs;
  }

  int32_t routerAdvertisementSeconds() const {
    return getNdpConfig()
        ->get<switch_config_tags::routerAdvertisementSeconds>()
        ->cref();
  }

  int32_t curHopLimit() const {
    return getNdpConfig()->get<switch_config_tags::curHopLimit>()->cref();
  }

  int32_t routerLifetime() const {
    return getNdpConfig()->get<switch_config_tags::routerLifetime>()->cref();
  }

  int32_t prefixValidLifetimeSeconds() const {
    return getNdpConfig()
        ->get<switch_config_tags::prefixValidLifetimeSeconds>()
        ->cref();
  }

  int32_t prefixPreferredLifetimeSeconds() const {
    return getNdpConfig()
        ->get<switch_config_tags::prefixPreferredLifetimeSeconds>()
        ->cref();
  }

  bool routerAdvertisementManagedBit() const {
    return getNdpConfig()
        ->get<switch_config_tags::routerAdvertisementManagedBit>()
        ->cref();
  }

  bool routerAdvertisementOtherBit() const {
    return getNdpConfig()
        ->get<switch_config_tags::routerAdvertisementOtherBit>()
        ->cref();
  }

  std::optional<std::string> routerAddress() const {
    if (auto addr = getNdpConfig()->get<switch_config_tags::routerAddress>()) {
      return addr->cref();
    }
    return std::nullopt;
  }

  /*
   * Inherit the constructors required for clone().
   * This needs to be public, as std::make_shared requires
   * operator new() to be available.
   */
  inline static const int kDefaultMtu{1500};

 private:
  using Base::Base;
  friend class CloneAllocator;
};

template <typename NTable>
inline const std::shared_ptr<NTable> Interface::getNeighborTable() const {
  return this->template getNeighborEntryTable<typename NTable::AddressType>();
}

} // namespace facebook::fboss
