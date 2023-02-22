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

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/MacTable.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <map>
#include <set>
#include <string>

namespace facebook::fboss {

class SwitchState;
class MacTable;

namespace cfg {
class Vlan;
}

using DhcpV4OverrideMap = std::map<folly::MacAddress, folly::IPAddressV4>;
using DhcpV6OverrideMap = std::map<folly::MacAddress, folly::IPAddressV6>;

class Vlan;
USE_THRIFT_COW(Vlan);
RESOLVE_STRUCT_MEMBER(Vlan, switch_state_tags::arpTable, ArpTable)
RESOLVE_STRUCT_MEMBER(Vlan, switch_state_tags::ndpTable, NdpTable)
RESOLVE_STRUCT_MEMBER(
    Vlan,
    switch_state_tags::arpResponseTable,
    ArpResponseTable)
RESOLVE_STRUCT_MEMBER(
    Vlan,
    switch_state_tags::ndpResponseTable,
    NdpResponseTable)
RESOLVE_STRUCT_MEMBER(Vlan, switch_state_tags::macTable, MacTable)

class Vlan : public ThriftStructNode<Vlan, state::VlanFields> {
 public:
  using Base = ThriftStructNode<Vlan, state::VlanFields>;
  using MemberPorts = std::map<int16_t, bool>;

  Vlan(VlanID id, std::string name);
  Vlan(const cfg::Vlan* config, MemberPorts ports);

  VlanID getID() const {
    return static_cast<VlanID>(get<switch_state_tags::vlanId>()->cref());
  }

  std::string getName() const {
    return get<switch_state_tags::vlanName>()->cref();
  }

  InterfaceID getInterfaceID() const {
    return static_cast<InterfaceID>(get<switch_state_tags::intfID>()->cref());
  }

  void setInterfaceID(InterfaceID intfID) {
    set<switch_state_tags::intfID>(intfID);
  }

  void setName(std::string name) {
    set<switch_state_tags::vlanName>(name);
  }

  // THRIFT_COPY: evaluate if this function can be changed..
  MemberPorts getPorts() const {
    return get<switch_state_tags::ports>()->toThrift();
  }
  // THRIFT_COPY: evaluate if this function can be changed..
  void setPorts(MemberPorts ports) {
    set<switch_state_tags::ports>(std::move(ports));
  }

  Vlan* modify(std::shared_ptr<SwitchState>* state);

  void addPort(PortID id, bool tagged);

  template <typename NTable>
  const std::shared_ptr<NTable> getNeighborTable() const;

  const std::shared_ptr<ArpTable> getArpTable() const {
    return get<switch_state_tags::arpTable>();
  }
  void setArpTable(std::shared_ptr<ArpTable> table) {
    ref<switch_state_tags::arpTable>() = std::move(table);
  }
  void setNeighborTable(std::shared_ptr<ArpTable> table) {
    ref<switch_state_tags::arpTable>() = std::move(table);
  }

  const std::shared_ptr<ArpResponseTable> getArpResponseTable() const {
    return get<switch_state_tags::arpResponseTable>();
  }
  void setArpResponseTable(std::shared_ptr<ArpResponseTable> table) {
    ref<switch_state_tags::arpResponseTable>() = std::move(table);
  }

  const std::shared_ptr<NdpTable> getNdpTable() const {
    return get<switch_state_tags::ndpTable>();
  }
  void setNdpTable(std::shared_ptr<NdpTable> table) {
    ref<switch_state_tags::ndpTable>() = std::move(table);
  }
  void setNeighborTable(std::shared_ptr<NdpTable> table) {
    ref<switch_state_tags::ndpTable>() = std::move(table);
  }

  const std::shared_ptr<NdpResponseTable> getNdpResponseTable() const {
    return get<switch_state_tags::ndpResponseTable>();
  }
  void setNdpResponseTable(std::shared_ptr<NdpResponseTable> table) {
    ref<switch_state_tags::ndpResponseTable>() = std::move(table);
  }

  // dhcp relay

  folly::IPAddressV4 getDhcpV4Relay() const {
    return folly::IPAddressV4(get<switch_state_tags::dhcpV4Relay>()->cref());
  }
  void setDhcpV4Relay(folly::IPAddressV4 v4Relay) {
    set<switch_state_tags::dhcpV4Relay>(v4Relay.str());
  }

  folly::IPAddressV6 getDhcpV6Relay() const {
    return folly::IPAddressV6(get<switch_state_tags::dhcpV6Relay>()->cref());
  }
  void setDhcpV6Relay(folly::IPAddressV6 v6Relay) {
    set<switch_state_tags::dhcpV6Relay>(v6Relay.str());
  }

  // dhcp overrides
  // THRIFT_COPY: evaluate if this function can be changed..
  DhcpV4OverrideMap getDhcpV4RelayOverrides() const {
    DhcpV4OverrideMap overrideMap{};
    for (auto iter :
         std::as_const(*get<switch_state_tags::dhcpRelayOverridesV4>())) {
      overrideMap.emplace(
          folly::MacAddress(iter.first),
          folly::IPAddressV4(iter.second->cref()));
    }
    return overrideMap;
  }
  // THRIFT_COPY: evaluate if this function can be changed..
  void setDhcpV4RelayOverrides(DhcpV4OverrideMap map) {
    std::map<std::string, std::string> overrideMap{};
    for (auto iter : map) {
      overrideMap.emplace(iter.first.toString(), iter.second.str());
    }
    set<switch_state_tags::dhcpRelayOverridesV4>(std::move(overrideMap));
  }
  // THRIFT_COPY: evaluate if this function can be changed..
  DhcpV6OverrideMap getDhcpV6RelayOverrides() const {
    DhcpV6OverrideMap overrideMap{};
    for (auto iter :
         std::as_const(*get<switch_state_tags::dhcpRelayOverridesV6>())) {
      overrideMap.emplace(
          folly::MacAddress(iter.first),
          folly::IPAddressV6(iter.second->cref()));
    }
    return overrideMap;
  }
  // THRIFT_COPY: evaluate if this function can be changed..
  void setDhcpV6RelayOverrides(DhcpV6OverrideMap map) {
    std::map<std::string, std::string> overrideMap{};
    for (auto iter : map) {
      overrideMap.emplace(iter.first.toString(), iter.second.str());
    }
    set<switch_state_tags::dhcpRelayOverridesV6>(std::move(overrideMap));
  }

  /*
   * TODO - replace getNeighborEntryTable as getNeighborTable
   * replace getNeighborTable<NTable> as getNeighborTable<NTable::AddressType>
   */
  template <typename AddrT>
  inline const std::shared_ptr<std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      ArpTable>>
  getNeighborEntryTable() const {
    return getArpTable();
  }

  template <typename AddrT>
  inline const std::shared_ptr<std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV6>::value,
      NdpTable>>
  getNeighborEntryTable() const {
    return getNdpTable();
  }

  const std::shared_ptr<MacTable> getMacTable() const {
    return get<switch_state_tags::macTable>();
  }

  void setMacTable(std::shared_ptr<MacTable> macTable) {
    ref<switch_state_tags::macTable>() = std::move(macTable);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

template <typename NTable>
inline const std::shared_ptr<NTable> Vlan::getNeighborTable() const {
  return this->template getNeighborEntryTable<typename NTable::AddressType>();
}

} // namespace facebook::fboss
