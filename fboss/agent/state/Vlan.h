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
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/MacTable.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NodeBase.h"
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

typedef boost::container::flat_map<folly::MacAddress, folly::IPAddressV4>
    DhcpV4OverrideMap;
typedef boost::container::flat_map<folly::MacAddress, folly::IPAddressV6>
    DhcpV6OverrideMap;

struct VlanFields {
  struct PortInfo {
    explicit PortInfo(bool emitTags) : tagged(emitTags) {}
    bool operator==(const PortInfo& other) const {
      return tagged == other.tagged;
    }
    bool operator!=(const PortInfo& other) const {
      return !(*this == other);
    }
    folly::dynamic toFollyDynamic() const;
    static PortInfo fromFollyDynamic(const folly::dynamic& json);
    bool tagged;
  };
  typedef boost::container::flat_map<PortID, PortInfo> MemberPorts;

  VlanFields(VlanID id, std::string name);
  VlanFields(
      VlanID id,
      std::string name,
      InterfaceID intfID,
      folly::IPAddressV4 dhcpV4Relay,
      folly::IPAddressV6 dhcpV6Relay,
      MemberPorts&& ports);

  template <typename Fn>
  void forEachChild(Fn fn) {
    fn(arpTable.get());
    fn(arpResponseTable.get());
    fn(ndpTable.get());
    fn(ndpResponseTable.get());
    fn(macTable.get());
  }

  folly::dynamic toFollyDynamic() const;
  static VlanFields fromFollyDynamic(const folly::dynamic& vlanJson);

  const VlanID id{0};
  std::string name;
  InterfaceID intfID{0};
  // DHCP server IP for the DHCP relay
  folly::IPAddressV4 dhcpV4Relay;
  folly::IPAddressV6 dhcpV6Relay;
  DhcpV4OverrideMap dhcpRelayOverridesV4;
  DhcpV6OverrideMap dhcpRelayOverridesV6;
  // The list of ports in the VLAN.
  // We only store PortIDs, and not pointers to the actual Port objects.
  // This way VLAN objects don't need to change when a Port object is modified.
  //
  // (Port state is copy-on-write, so when it changes a new copy of the Port
  // object is created.  If we pointed at the Port object here we would also
  // have to modify the Vlan object.  By storing only the PortID the Vlan does
  // not need to be modified.)
  MemberPorts ports;
  std::shared_ptr<ArpTable> arpTable;
  std::shared_ptr<ArpResponseTable> arpResponseTable;
  std::shared_ptr<NdpTable> ndpTable;
  std::shared_ptr<NdpResponseTable> ndpResponseTable;
  std::shared_ptr<MacTable> macTable;
};

class Vlan : public NodeBaseT<Vlan, VlanFields> {
 public:
  typedef VlanFields::PortInfo PortInfo;
  typedef VlanFields::MemberPorts MemberPorts;

  Vlan(VlanID id, std::string name);
  Vlan(const cfg::Vlan* config, MemberPorts ports);

  static std::shared_ptr<Vlan> fromFollyDynamic(const folly::dynamic& json) {
    const auto& fields = VlanFields::fromFollyDynamic(json);
    return std::make_shared<Vlan>(fields);
  }

  static std::shared_ptr<Vlan> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  VlanID getID() const {
    return getFields()->id;
  }

  const std::string& getName() const {
    return getFields()->name;
  }

  InterfaceID getInterfaceID() const {
    return getFields()->intfID;
  }

  void setInterfaceID(InterfaceID intfID) {
    writableFields()->intfID = intfID;
  }

  void setName(std::string name) {
    writableFields()->name = name;
  }

  const MemberPorts& getPorts() const {
    return getFields()->ports;
  }
  void setPorts(MemberPorts ports) {
    writableFields()->ports.swap(ports);
  }

  Vlan* modify(std::shared_ptr<SwitchState>* state);

  void addPort(PortID id, bool tagged);

  template <typename NTable>
  const std::shared_ptr<NTable> getNeighborTable() const;

  const std::shared_ptr<ArpTable> getArpTable() const {
    return getFields()->arpTable;
  }
  void setArpTable(std::shared_ptr<ArpTable> table) {
    return writableFields()->arpTable.swap(table);
  }
  void setNeighborTable(std::shared_ptr<ArpTable> table) {
    return writableFields()->arpTable.swap(table);
  }

  const std::shared_ptr<ArpResponseTable> getArpResponseTable() const {
    return getFields()->arpResponseTable;
  }
  void setArpResponseTable(std::shared_ptr<ArpResponseTable> table) {
    writableFields()->arpResponseTable.swap(table);
  }

  const std::shared_ptr<NdpTable> getNdpTable() const {
    return getFields()->ndpTable;
  }
  void setNdpTable(std::shared_ptr<NdpTable> table) {
    return writableFields()->ndpTable.swap(table);
  }
  void setNeighborTable(std::shared_ptr<NdpTable> table) {
    return writableFields()->ndpTable.swap(table);
  }

  const std::shared_ptr<NdpResponseTable> getNdpResponseTable() const {
    return getFields()->ndpResponseTable;
  }
  void setNdpResponseTable(std::shared_ptr<NdpResponseTable> table) {
    writableFields()->ndpResponseTable.swap(table);
  }

  // dhcp relay

  folly::IPAddressV4 getDhcpV4Relay() const {
    return getFields()->dhcpV4Relay;
  }
  void setDhcpV4Relay(folly::IPAddressV4 v4Relay) {
    writableFields()->dhcpV4Relay = v4Relay;
  }

  folly::IPAddressV6 getDhcpV6Relay() const {
    return getFields()->dhcpV6Relay;
  }
  void setDhcpV6Relay(folly::IPAddressV6 v6Relay) {
    writableFields()->dhcpV6Relay = v6Relay;
  }

  // dhcp overrides

  DhcpV4OverrideMap getDhcpV4RelayOverrides() const {
    return getFields()->dhcpRelayOverridesV4;
  }
  void setDhcpV4RelayOverrides(DhcpV4OverrideMap map) {
    writableFields()->dhcpRelayOverridesV4 = map;
  }

  DhcpV6OverrideMap getDhcpV6RelayOverrides() const {
    return getFields()->dhcpRelayOverridesV6;
  }
  void setDhcpV6RelayOverrides(DhcpV6OverrideMap map) {
    writableFields()->dhcpRelayOverridesV6 = map;
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
    return getFields()->macTable;
  }

  void setMacTable(std::shared_ptr<MacTable> macTable) {
    writableFields()->macTable.swap(macTable);
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

template <typename NTable>
inline const std::shared_ptr<NTable> Vlan::getNeighborTable() const {
  return this->template getNeighborEntryTable<typename NTable::AddressType>();
}

} // namespace facebook::fboss
